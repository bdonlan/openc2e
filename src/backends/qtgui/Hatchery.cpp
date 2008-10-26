/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "World.h"
#include "Hatchery.h"
#include "Engine.h"
#include "creaturesImage.h"
#include "mmapifstream.h"
#include "qtopenc2e.h"

#include <QtGui>

/*
  C1 hatchery resources:
  hatchery.bmp and htchmask.bmp used for the background/foreground
  SCAN*.BMP and hdsk.wav used for egg disk animation
  EGG*.BMP and hegg.wav used for egg movement animation
  FAN*.BMP and hfan.wav used for the fan animation
  lightoff.bmp and hlgt.wav used for the light flickering
  GENSPIN.BMP, and hmle.wav/hfml.wav used for male/female animation (male.bmp and female.bmp also present)

  C2's Omelette.s16:
  * frames 0-16 are eggs - 11-16 are masked with black
  * frame 17 is an egg shadow
  * frames 22-37 is a rotating questionmark anim
  * frames 38-53 (female) and 54-69 are (male) are rotating gender symbol anims
  *
  * frame 18 and 19 are sides of the hatchery
  * frame 20 is the grabber
  * frame 21 is the middle bit of the side
*/

/*
 * Construct a QImage from a given frame of a sprite, and make the colour
 * of the top-left pixel transparent. If makealpha is set, also apply
 * alpha transparency to the rest of the image.
 */
QImage imageFromSpriteFrame(shared_ptr<creaturesImage> img, unsigned int frame, bool makealpha = false) {
	assert(img->format() == if_16bit);

	// img->data is not 32-bit-aligned so we have to make a copy here.
	QImage ourimg = QImage(img->width(frame), img->height(frame),
			img->is565() ? QImage::Format_RGB16 : QImage::Format_RGB555);

	for (unsigned int i = 0; i < img->height(frame); i++) {
		memcpy(ourimg.scanLine(i), (uint16 *)img->data(frame) + (i * img->width(frame)), img->width(frame) * 2);
	}

	QImage mask = ourimg.createMaskFromColor(ourimg.pixel(0, 0), Qt::MaskOutColor);

	if (makealpha) {
		mask = mask.convertToFormat(QImage::Format_RGB32);
		for (unsigned int i = 0; i < (unsigned int)mask.height(); i++) {
			for (unsigned int j = 0; j < (unsigned int)mask.width(); j++) {
				// slow code is slow
				if (mask.pixel(j, i) != mask.pixel(0, 0))
					mask.setPixel(j, i, QColor(0x80, 0x80, 0x80).rgb());
			}
		}
	}

	ourimg.setAlphaChannel(mask);

	return ourimg;
}

class EggItem : public QGraphicsPixmapItem {
protected:
	int realx, realy;
	QGraphicsPixmapItem *gender;
	QGraphicsTextItem *details;
	Hatchery *hatchery;

public:
	EggItem(Hatchery *h, QPixmap i, int x, int y) : QGraphicsPixmapItem(i) {
		setAcceptHoverEvents(true);
		setPos(x, y);
		setZValue(y);
		
		hatchery = h;
		realx = x;
		realy = y;

		// TODO: store gender, actual egg details

		// TODO: animation, use QTimeLine?
		gender = new QGraphicsPixmapItem(hatchery->genderanim[0][0]);
		gender->setVisible(false);
		gender->setPos(x + (i.width() / 2) - (gender->pixmap().width() / 2), y - gender->pixmap().height());
		gender->setZValue(2000);
		hatchery->graphicsScene->addItem(gender);
		
		details = new QGraphicsTextItem("egg description here"); // TODO
		details->setVisible(false);
		//details->setPos(hatchery->graphicsScene->height() - details->height() - 5, hatchery->graphicsScene->width() - (details->width() / 2));
		details->setPos(0, 0);
		details->setZValue(2000);
		hatchery->graphicsScene->addItem(details);
	}

	void hoverEnterEvent(QGraphicsSceneHoverEvent *event) {
		gender->setVisible(true);
		details->setVisible(true);
	}

	void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) {
		gender->setVisible(false);
		details->setVisible(false);
	}

	void mousePressEvent(QGraphicsSceneMouseEvent* event) {
		event->accept();
	}

	void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) {
		gender->setVisible(false);
		details->setVisible(false);
		setVisible(false);

		// TODO: egg colour should match, provide gender/genome/etc details
		hatchery->qtopenc2e->makeNewEgg();
		hatchery->close();
	}
};

// TODO: these are from imageManager.cpp, it'd be nice to have a non-hacky interface,
// but we probably need to fix the image object model first due to endianism issues
enum filetype { blk, s16, c16, spr, bmp };
bool tryOpen(mmapifstream *in, shared_ptr<creaturesImage> &img, std::string fname, filetype ft);

Hatchery::Hatchery(QtOpenc2e *parent) : QDialog(parent) {
	qtopenc2e = parent;

	setWindowTitle(tr("Hatchery"));
	setAttribute(Qt::WA_QuitOnClose, false);

	/* hatchery background */
	std::string hatcherybgfile;
	if (engine.version == 1) hatcherybgfile = world.findFile("hatchery/hatchery.bmp");
	else if (engine.version == 2) hatcherybgfile = world.findFile("Applet Data/Hatchery.bmp");
	if (hatcherybgfile.empty()) return;

	QPixmap hatcherybg(QString(hatcherybgfile.c_str()));
	resize(hatcherybg.width() + 6, hatcherybg.height() + 6);
	
	/* create the widgets/layout */
	graphicsScene = new QGraphicsScene();
	graphicsView = new QGraphicsView(graphicsScene, this);
	QHBoxLayout *layout = new QHBoxLayout(this);
	layout->addWidget(graphicsView);
	layout->setContentsMargins(0, 0, 0, 0);
	
	graphicsScene->addPixmap(hatcherybg);

	if (engine.version == 1) {
		/* mask which goes over the eggs */
		std::string hatcherymaskfile = world.findFile("hatchery/htchmask.bmp");
		if (hatcherymaskfile.size()) {
			QPixmap hatcherymask(QString(hatcherymaskfile.c_str()));
			QColor maskcolour(0xff, 0x00, 0x80);
			hatcherymask.setMask(hatcherymask.createMaskFromColor(maskcolour));
	
			QGraphicsPixmapItem *maskitem = graphicsScene->addPixmap(hatcherymask);
			maskitem->setPos(0, 168);
		}

		/* fan animation */
		for (unsigned int i = 0; i < 4; i++) {
			// TODO
		}
	
		/* 'off' state for the light */
		// TODO
	
		/* eggs */
		// TODO
	
		/* gender marker animation */
		// TODO
	} else { // C2
		mmapifstream *s = new mmapifstream();
		if (!tryOpen(s, omelettedata, "Applet Data/Omelette.s16", s16)) {
			return;
		}

		for (unsigned int i = 0; i < 3; i++) {
			for (unsigned int j = 0; j < 16; j++) {
				genderanim[i][j] = QPixmap::fromImage(imageFromSpriteFrame(omelettedata, 22 + (i * 16) + j));
			}
		}

		QGraphicsPixmapItem *item;
		
		QImage leftsideimg = imageFromSpriteFrame(omelettedata, 18, true);
		item = graphicsScene->addPixmap(QPixmap::fromImage(leftsideimg));
		item->setPos(75, 176);
		item->setZValue(1000);

		QImage rightsideimg = imageFromSpriteFrame(omelettedata, 19, true);
		item = graphicsScene->addPixmap(QPixmap::fromImage(rightsideimg));
		item->setPos(181, 162);
		item->setZValue(1000);
		
		QImage grabberimg = imageFromSpriteFrame(omelettedata, 20);
		item = graphicsScene->addPixmap(QPixmap::fromImage(grabberimg));
		item->setPos(150, 93);
		item->setZValue(1000);
		
		QImage midsideimg = imageFromSpriteFrame(omelettedata, 21);
		item = graphicsScene->addPixmap(QPixmap::fromImage(midsideimg));
		item->setPos(141, 177);	
		item->setZValue(1000);

		QImage eggimg[17];
		EggItem *eggitems[17];

		for (unsigned int i = 0; i < 10; i++) {
			eggimg[i] = imageFromSpriteFrame(omelettedata, i);

			// Place eggs from 80-240 horizontally, 110-230 vertically
			int x = 0, y = 0, tries = 0;
			while (x == 0 && tries < 50) {
				x = (rand() / (RAND_MAX + 1.0)) * (240 - 80 - eggimg[i].width()) + 80;
				y = (rand() / (RAND_MAX + 1.0)) * (230 - 110 - eggimg[i].height()) + 110;
				for (unsigned int j = 0; j < i; j++) {
					float xdist = x - eggitems[j]->x(); xdist = xdist * xdist;
					float ydist = y - eggitems[j]->y(); ydist = ydist * ydist;
					if (xdist + ydist < 20 * 20) { // less than 20 pixels apart
						x = 0; // try again
					}
				}
				tries++; // give up after 50 attempts at placement..
			}
			
			item = eggitems[i] = new EggItem(this, QPixmap::fromImage(eggimg[i]), x, y);
			graphicsScene->addItem(item);

			// TODO: place shadows
		}
	}
}

Hatchery::~Hatchery() {
}

