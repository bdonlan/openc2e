#!/usr/bin/perl

# todo: handle XYX# as well as XYX:

my @files = <../caosVM_*.cpp>;

my %doublecmds;
my %doublefuncs;

my $parsingdoc = 0;
my $doclines;

my %funcs;
my %voids;

my $data;

sub writedata {
	my ($name, $docline, $type, $newname) = @_;

	my @sp = split(/\(/, $docline);
  my $count = int(@sp) - 2;

	die("unknown type $type passed to writedata") if ($type != 1) && ($type != 2);
	
	if ($newname eq $name) {
		$data = $data . "CMDDEF(" . $name if ($type == 1);
		$data = $data . "FUNCDEF(" . $name if ($type == 2);
		$count = 0 if $docline =~ /condition/;
    $data = $data . ", " . $count . ")\n";
		if ($docline =~ /condition/) {
			die("only commands can have conditions while processing " . $name) unless ($type == 1);
			$data = $data . 'cmds[phash_cmd(*(int *)"' . $name . '")].needscondition = true;' . "\n";
		}				
	} else {
		my $firstname = $newname;
		$firstname =~ s/(.*) .*/$1/;
														
	  if ($type == 1) {
			if (!$doublecmds{$firstname}) {
				$data = $data . "DBLCMDDEF(\"" . $firstname . "\")\n";
				$doublecmds{$firstname} = 1;
			}
			$data = $data . "DBLCMD(\"";
		} elsif ($type == 2) {
			if (!$doublefuncs{$firstname}) {
				$data = $data . "DBLFUNCDEF(\"" . $firstname . "\")\n";
				$doublefuncs{$firstname} = 1;
			}
			$data = $data . "DBLFUNC(\"";
		}
		$data = $data . $newname . "\", " . $name . ", " . $count . ")\n";
  }
}

sub writedocsanddata {
	my ($name, $type) = @_;

	my $newname = $name;
	if ($name =~ /_/) {
		my $one = $name;
		$one =~ s/(.*)_(.*)/$1/;
		my $two = $name;
		$two =~ s/(.*)_(.*)/$2/;
		if (length($one) == 3) {
			$one = $one . ":";
		}
		$newname = $one . " " . $two;
	}
	if ($doclines) {
    my $firstline = (split(/\n/, $doclines))[0];
		$firstline =~ s/^\s*(.*)\s*$/$1/;
		writedata($name, $firstline, $type, $newname);
		
		$doclines =~ s/\n/<br>/;
		print docfile "<h2>", $newname, "</h2>";
		print docfile "<p>", $doclines, "</p>\n";
	} else {
	  print "command/function '", $newname, "' wasn't processed because it has no documentation. add at least a prototype.\n";
	}
	
	$doclines = "";
}

open(docfile, ">caosdocs.html");

foreach my $fname (@files) {
	open(filehandle, $fname);
	while(<filehandle>) {
		if (/^void caosVM::/) {
			my $type = 0;
			$_ =~ s/^void caosVM:://;
			$_ =~ s/\(\) {$//;
			$_ =~ s/\n//;
			if (/^c_/) {
				$_ =~ s/^c_//;
				$type = 1;
			} elsif (/^v_/) {
				$_ =~ s/^v_//;
				$type = 2;
			} else {
				printf "failed to understand " . $_ . "\n";
				next;
			}
			writedocsanddata($_, $type);
			$_ =~ s/(.*)(_.*)/$1/;
			if (length($_) == 3) {
				$_ = $_ . ":";
			}
			if ($type == 1) {
				$voids{$_} = 1;
			} elsif ($type == 2) {
				$funcs{$_} = 1;
			}
		} elsif (/^\/\*\*$/) {
			$parsingdoc = 1;
		} elsif (/\*\//) {
			$parsingdoc = 0;
		} elsif ($parsingdoc) {
			$doclines = $doclines . $_;
		}
	}
	close(filehandle);
}

close(docfile);

# now, generate prehash files

open(outhandle, ">prehash_cmds.txt");
foreach my $key (keys(%voids)) {
	print outhandle $key, "\n";
}
close(outhandle);
open(outhandle, ">prehash_funcs.txt");
foreach my $key (keys(%funcs)) {
	print outhandle $key, "\n";
}
close(outhandle);

# then, generate caosdata.cpp

open(outhandle, ">caosdata.cpp");
open(templatehandle, "caosdata_template.cpp");
my $template;
while (<templatehandle>) {
	$template .= $_;
}
close(templatehandle);
$template =~ s/__CAOSGENERATED__/Automatically generated by makedocs.pl./;
$template =~ s/__CAOSMACROS__/$data/;
print outhandle $template;
close(outhandle);
