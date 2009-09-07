#!/usr/bin/perl
# Read include file, and generate NF function calls

$file = @ARGV[0];

if ( ! defined(open(FILE, $file)) ) {
	warn "Couldn't open $file: $!\n";
	exit;
}

print "/* Generated by lib-gen.pl from $file */\n\n";

#print "\t.text\n";
#print "\t.even\n";

$linecount=0;
while ($ligne = <FILE>) {
	if ($ligne =~ /^GLAPI/ ) {
		while (! ($ligne =~ /\);/)) {
			chomp($ligne);
			$ligne .= " " . <FILE>;
		}
		$ligne =~ s/\t//g;
		$ligne =~ s/\n//g;
		$ligne =~ s/ +/ /g;

		# Add missing parameters (for glext.h)
		$lettre = 'a';
		while ($ligne =~ /[,\( ] *GL\w+\** *\**,/ ) {
			$ligne =~ s/([,\( ] *GL\w+\** *\**),/$1 $lettre,/;
			$lettre++;
		}
		while ($ligne =~ /[,\( ] *GL\w+\** *\**\)/ ) {
			$ligne =~ s/([,\( ] *GL\w+\** *\**)\)/$1 $lettre\)/;
			$lettre++;
		}
		while ($ligne =~ /[,\( ] *GL\w+\** *const *\*,/ ) {
			$ligne =~ s/([,\( ] *GL\w+\** *const *\*),/$1 $lettre,/;
			$lettre++;
		}

		if ($ligne =~ /^GLAPI *(\w+).* (GL)*APIENTRY *(\w+) *\(.*/) {
			$return_type = $1 ;
			$function_name = "NFOSMESA_" . $3 ;
			$copy_function_name = $3;
		} else {
			$return_type = "" ;
			$function_name = "" ;
			$copy_function_name = "";
		}

		$prototype = $ligne;
		$prototype =~ s/GLAPI +//;
		$prototype =~ s/(GL)APIENTRY +//;
		$prototype =~ s/\;.*//;
		# Remove start of line
		$ligne =~ s/^GLAPI.*(GL)*APIENTRY +// ;
		$ligne =~ s/\;.*// ;
		$ligne =~ s/.*(\(.*\))/$1/ ;

		# Remove parameter types
		if ( $ligne =~ /\( *(void) *\)/ ) {
			# Remove void list of parameters
			$ligne =~ s/\(.*\)/NULL/;
		} else {
			# Remove parameters type
			while ( $ligne =~ /[,\(] *((const)* *(\w)+ *\**) +\**((\w)+) *[,\)]/ ) {
				$ligne =~ s/$1/$2/;
			}
			$ligne =~ s/\*//g;

			$ligne =~ s/ +//g;
			$ligne =~ s/\((.*)\)/$1/;
			$ligne = "," . $ligne;
		}

		chomp($ligne);
		$function_name =~ tr/a-z/A-Z/;

		# Keep first parameter
		while ( $ligne =~ /,.*,(\w+)/) {
			$ligne =~ s/,$1$//;
		}
		$ligne =~ s/,/&/;

		if ($prototype !~ /^void \w/) {
			$return_type = "(" . $return_type . ")";
		} else {
			$return_type = "";
		}
		$ligne = $return_type . "(*HostCall_p)(" . $function_name .",cur_context," . $ligne .");";
		if ( $prototype !~ /^void \w/ ) {
			$ligne = "return " . $ligne;
		}

		print "$prototype\n";
		print "{\n";
		print "\t$ligne\n";
		print "}\n\n";

#		print "\t.globl\t_$copy_function_name\n";
#		print "_$copy_function_name:\n";
#		print "\tpea\tsp@(4)\n";
#		print "\tmovel\t_cur_context,sp@-\n";
#		print "\tpea\t$function_name:w\n";
#		print "\tmovel\t_HostCall_p,a0\n";
#		print "\tjbsr\ta0@\n";
#		print "\tlea\tsp@(12),sp\n";
#		print "\trts\n";

		$linecount++;
	}
}
close(FILE);
print "/* Functions generated: $linecount */\n";