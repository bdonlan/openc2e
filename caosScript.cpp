/*
 *  caosScript.cpp
 *  openc2e
 *
 *  Created by Alyssa Milburn on Wed May 26 2004.
 *  Copyright (c) 2004 Alyssa Milburn. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 */

#include "caosVM.h"
#include "openc2e.h"
#include "World.h"
#include <iostream>
#include <sstream>

std::string token::dump() {
	if (comparison == NONE) return (isvar ? var.dump() : ((cmd == 0) ? ((func == 0) ? "[bad command!] " : func->dump()) : cmd->dump()));
	else {
		switch (comparison) {
			case EQ: return "EQ ";
			case NE: return "NE ";
			case GT: return "GT ";
			case GE: return "GE ";
			case LT: return "LT ";
			case LE: return "LE ";
			case AND: return "AND ";
			case OR: return "OR ";
			default: return "[bad comparison!] ";
		}
	}
}

inline std::string stringify(double x) {
	std::ostringstream o;
	if (!(o << x)) throw "stringify() failed";
	return o.str();
}

std::string caosVar::dump() {
	std::string out = "[caosVar: ";
	if (hasString()) {
		out += std::string("\"") + stringValue + "\"";
	} else if (hasInt()) {
		out += stringify(intValue);
		out += "(i)";
	} else if (hasFloat()) {
		out += stringify(floatValue);
		out += "(f)";
	} else {
		out += "unknown flags: ";
		out += flags;
	}
	out += "] ";
	return out;
}

std::string cmdinfo::dump() {
	return std::string("[command: ") + name + "] ";
}

#define _SHOW_RAWLINES
std::string script::dump() {
	std::string out;
	for (unsigned int i = 0; i < lines.size(); i++) {
#ifdef _SHOW_RAWLINES
		out += std::string("[rawline: ") + rawlines[i] + "]\n";
#endif
		for (std::list<token>::iterator j = lines[i].begin(); j != lines[i].end(); j++) {
			out += j->dump();
		}
#ifdef _SHOW_RAWLINES
		out += "\n\n";
#else
		out += "\n";
#endif
	}
	return out;
}

std::string caosScript::dump() {
	std::string out = "installation script:\n" + installer.dump();
	out += "removal script:\n" + removal.dump();
	for (std::vector<script>::iterator i = scripts.begin(); i != scripts.end(); i++) {
		out += "agent script:\n" + i->dump();
	}
	return out;
}

// TODO: debug use only?
std::map<std::string, bool> seenbadsymbols;

token makeToken(std::string &src, bool str, token &lasttok) {
	token r;
	// todo: hrr. we shouldn't be doing this on every tokenisation pass
	if ((lasttok.cmd) && ( (lasttok.cmd == getCmdInfo("GSUB", true)) || (lasttok.cmd == getCmdInfo("SUBR", true))) ) {
		r.var.setString(src);
		return r;
	}
	if (str) { // handle strings (and arrays, at present)
		r.var.setString(src);
	} else if ((isdigit(src[0])) || (src[0] == '.') || (src[0] == '-')) { // handle digits
		if (src.find(".") == std::string::npos) r.var.setInt(atoi(src.c_str()));
		else r.var.setFloat(atof(src.c_str()));
//		if (errno == EINVAL) throw tokeniseFailure();
		// TODO: check for zero return value from atoi (and atof?) and then check for EINVAL
	} else if (src.size() == 4) { // handle commands
		r.isvar = false;
		cmdinfo *lastcmd = (lasttok.cmd ? lasttok.cmd : lasttok.func);
		if ((!lasttok.isvar) && (lastcmd->twotokens)) {
			if (lasttok.cmd) r.cmd = getSecondCmd(lastcmd, src, true);
			else r.func = getSecondCmd(lastcmd, src, false);
		} else {
			r.cmd = getCmdInfo(src, true);
			r.func = getCmdInfo(src, false);
			// this is a global hack from caosVM_cmdinfo for VAxx/OVxx
			if (varnumber != -1) r.varnumber = varnumber;
		}
		if (!r.cmd && !r.func) {
			if (!seenbadsymbols[src]) {
				std::cerr << "caosScript parser failed to find presumed function \"" << src << "\": each missing function is reported only once\n";
				seenbadsymbols[src] = true;
			}

			throw tokeniseFailure();
		}
	} else { // presumably we have a comparison
		transform(src.begin(), src.end(), src.begin(), toupper);
		// todo: make this a hash table?
		if (src == "NE") r.comparison = NE;
		else if (src == "EQ") r.comparison = EQ;
		else if (src == "GE") r.comparison = GE;
		else if (src == "GT") r.comparison = GT;
		else if (src == "LE") r.comparison = LE;
		else if (src == "LT") r.comparison = LT;
		else if (src == "<>") r.comparison = NE;
		else if (src == "=") r.comparison = EQ;
		else if (src == ">=") r.comparison = GE;
		else if (src == ">") r.comparison = GT;
		else if (src == "<=") r.comparison = LE;
		else if (src == "<") r.comparison = LT;
		else if (src == "AND") r.comparison = AND;
		else if (src == "OR") r.comparison = OR;
		else throw tokeniseFailure();
	}
	return r;
}

void tokenise(std::string s, std::list<token> &t) {
	std::string currtoken;
	token lasttoken;
	for (std::string::iterator i = s.begin(); i != s.end(); i++) {
		if ((*i == ' ') || (*i == '\t') || (*i == '\r')) {
			if (!currtoken.empty()) {
				lasttoken = makeToken(currtoken, false, lasttoken);
				if ((lasttoken.isvar) || (lasttoken.cmd ? !lasttoken.cmd->twotokens : !lasttoken.func->twotokens))
					t.push_back(lasttoken);
				currtoken.clear();
			}
		} else if (*i == '"') {
			if (!currtoken.empty()) throw tokeniseFailure();
			i++; // skip the "
			while (*i != '"') {
				currtoken += *i;
				i++;
				if (i == s.end()) throw tokeniseFailure();
			}
			t.push_back(makeToken(currtoken, true, lasttoken));
			currtoken.clear();
		} else if (*i == '[') {
			if (!currtoken.empty()) throw tokeniseFailure();
			i++; // skip the [
			while (*i != ']') {
				currtoken += *i;
				i++;
				if (i == s.end()) throw tokeniseFailure();
			}
			t.push_back(makeToken(currtoken, true, lasttoken));
			currtoken.clear();
		} else if (*i == '*') {
			// start of a comment. forget the rest of the line.
			i = s.end() - 1;
		} else {
			currtoken += *i;
		}
	}
	if (!currtoken.empty()) {
		t.push_back(makeToken(currtoken, false, lasttoken));
	}
}

caosScript::caosScript(std::istream &in) {
	std::vector<std::list<token> > lines;
	std::vector<std::string> rawlines;

	int lineno = 0;
	while (!in.fail()) {
		lineno++;
		std::list<token> t;
		std::string s;
		std::getline(in, s);
		if (s[s.size() - 1] == '\r')
			s.erase(s.end() - 1);
		try {
			tokenise(s, t);
			if (!t.empty()) {
				lines.push_back(t);
				rawlines.push_back(s);
			}
		} catch (tokeniseFailure f) {
			// std::cerr << "failed to tokenise line #" << lineno << "(" << s << ")\n";
		}
	}

	// we don't find scrp tokens which aren't on seperate lines here.
	// the fix for this is probably to split things up between lines
	// at tokenisation time..

	/*
	  okay, here we go through the scrip we've parsed, and strip out
	  the individual script elements - ie, installation script, removal
	  script, and agent scripts
	*/
	cmdinfo *scrp = getCmdInfo("SCRP", true); assert(scrp != 0);
	cmdinfo *rscr = getCmdInfo("RSCR", true); assert(rscr != 0);
	cmdinfo *endm = getCmdInfo("ENDM", true); assert(endm != 0);
	script *currscrip = &installer;
	
	for (unsigned int i = 0; i < lines.size(); i++) {
		std::list<token> &l = lines[i];
		if (l.front().cmd != 0) {
			if (l.front().cmd == scrp) {
				assert(l.size() == 5);
				int one, two, three, four;
				std::list<token>::iterator i = l.begin();
				// TODO: shouldn't add scripts here, should store them for optional addition
				i++; assert(i->isvar); assert(i->var.hasInt()); one = i->var.intValue;
				i++; assert(i->isvar); assert(i->var.hasInt()); two = i->var.intValue;
				i++; assert(i->isvar); assert(i->var.hasInt()); three = i->var.intValue;
				i++; assert(i->isvar); assert(i->var.hasInt()); four = i->var.intValue;
				std::cout << "caosScript: script " << one << " " << two << " " << three << " " << four
					<< " being added to scriptorium.\n";
				currscrip = &(world.scriptorium.getScript(one, two, three, four));
				// todo: verify event script doesn't already exist, maybe? don't know
				// what real engine does
			} else if (l.front().cmd == rscr) {
				currscrip = &removal;
			} else if (l.front().cmd == endm) {
				currscrip->lines.push_back(l);
				currscrip->rawlines.push_back(rawlines[i]);
				currscrip = &installer;
			} else {
				currscrip->lines.push_back(l);
				currscrip->rawlines.push_back(rawlines[i]);
			}
		} else {
			std::cerr << "skipping rawline '" << rawlines[i] << "' because first parsed token wasn't a command (this is likely to be a bug in openc2e)\n";
		}
	}
}
