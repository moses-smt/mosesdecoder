Summary:
	PURPOSE
	PERFORMANCE
	REQUIREMENTS
	INSTALLATION
	HOW TO USE
	GETTING THE RESULTS
	THANKS
	LICENSE


********************************************************************************
PURPOSE:
********************************************************************************
This is the MS Windows and Linux version (tested with Ubuntu 10.10 and 11.04)
of Extract_Tmx_Corpus_1.044.

Extract_Tmx_Corpus_1.044 was created initially as a Windows program (tested in
Windows 7, Vista and XP) with a view to enable translators not necessarily with
a deep knowledge of linguistic tools to create highly customised corpora that
can be used with the Moses machine translation system and with other systems.
Some users call it "et cetera", playing a bit with its initials (ETC) and
meaning that it can treat a never-ending number of files.

In order to create corpora that are most useful to train machine translation
systems, one should strive to include segments that are relevant for the task in
hand. One of the ways of finding such segments could involve the usage of
previous translation memory files (TMX files). This way the corpora could be
customised for the person or for the type of task in question. The present
program uses such files as input.

The program can create strictly aligned corpora for a single pair of languages,
several pairs of languages or all the pairs of languages contained in the TMX
files.

The program creates 2 separate files (UTF-8 format; Unix line endings) for each
language pair that it processes: one for the starting language and another for
the destination language. The lines of a given TMX translation unit are placed
in strictly the same line in both files. The program suppresses empty TMX
translation units, as well as those where the text for the first language is the
same as that of the second language (like translation units consisting solely of
numbers, or those in which the first language segment has not been translated
into the second language). If you are interested in another format of corpus, it
should be relatively easy to adapt this format to the format you are interested
in.

The program also informs about errors that might occur during processing and
creates a file that lists the name(s) of the TMX files that caused them, as well
as a separate one listing the files successfully treated and the number of
segments extracted for the language pair.

********************************************************************************
REQUIREMENTS:
********************************************************************************
The program requires the following to be pre-installed in your computer:

1) Python 2.5 or higher (The program has been tested on Python 2.5 to 2.7.)
	Windows users download and install from http://www.python.org/download/
	Ubuntu users can use the pre-installed Python distribution

2) wxPython 2.8 or higher
	Windows users download and install the Unicode version from
	http://www.wxpython.org/download.php
	Ubuntu users install with:
		sudo apt-get install python-wxtools

3) Pythoncard 0.8.2 or higher
	Windows users download and install
	http://sourceforge.net/projects/pythoncard/files/PythonCard/0.8.2/PythonCard-0.8.2.win32.exe/download
	Ubuntu/Debian users install with:
		sudo apt-get install pythoncard

********************************************************************************
INSTALLATION:
********************************************************************************
Windows users:
1) Download the Extract_TMX_Corpus_1.041.exe file
2) Double-click Extract_TMX_Corpus_1.041.exe and follow the wizard's
   instructions.
NOTE: Windows Vista users, to run the installation programs, by right-click on
the installation file in Windows Explorer and choose "Execute as administrator"
in the contextual menu.

Ubuntu users:
1) Download the Moses2TMX.tgz compressed file to a directory of your choice.
2) Expand the compressed file and run from the expanded directory.

***IMPORTANT***: Never erase the file "LanguageCodes.txt" in that directory. It
is necessary for telling the program the languages that it has to process. If
your TMX files use language codes that are different from those contained in
this file, please replace the codes contained in the file with the codes used in
your TMX files. You can always add or delete new codes to this file (when the
program is not running).

********************************************************************************
HOW TO USE:
********************************************************************************
1) Create a directory where you will copy the TMX files that you want to 
   process.

2) Copy the TMX files to that directory.
Note: If you do not have TMX files, try the following site:
http://langtech.jrc.it/DGT-TM.html#Download. It contains the European Union
DGT's Translation Memory, containing legislative documents of the European
Union. For more details, see http://wt.jrc.it/lt/Acquis/DGT_TU_1.0/data/. These
files are compressed in zip format and need to be unzipped before they can be
used.

3) Launch the program.

4) Operate on the main window of the program in the direction from top to 
   bottom:

	a) Click the "Select input/output directory" button to tell the root
	directory where the TMX files are (this directory can have subdirectories,
	all of which will also be processed), as well as where the output files
	produced by the program will be placed;
	NOTE: Please take note of this directory because the result files will also
	be placed there.

	b) In case you want to extract a ***single*** pair of languages, choose them
	in the "Starting Language" and "Destination Language" comboboxes. Do nothing
	if you want to extract more than one pair of languages.

	c) Click the "Select base name of output file" button and choose a base name
	for the output files (default: "Corpus.txt").
	Note: This base name is used to compose the names of the output files, which
	will also include the names of the starting and destination languages. If
	you accept the default "Corpus.txt" and choose "EN-GB" as starting language
	and "PT-PT" as destination language, for that corpus pair the respective
	corpora files will be named, respectively, "EN-GB (PT-PT)_Corpus.txt" and
	"PT-PT (EN-GB)_Corpus.txt".
	***TIP***: The base name is useful for getting different names for different
	corpora of the same language.

	d) Click one (***just one***) of the following buttons:
		- "Extract one corpus": this creates a single pair of strictly aligned
		corpora in the languages chosen in the "Starting Language" and
		"Destination Language" comboboxes;
		- "Extract all corpora": this extracts all the combination pairs of
		languages for all the languages available in the "Starting Language" and
		"Destination language" comboboxes; if a language pair does not have
		segments of both languages in all of the translation units of all the
		TMX files, the result will be two empty corpora files for that language
		pair. If, however, there is just a single relevant translation unit, the
		corpus won't be empty.
		- "Extract some corpora": this extracts the pairs of languages listed in
		the file "LanguagePairs.txt". Each line of this file has the following
		structure:
			{Starting Language}/{Destination Language}.

Here is an example of a file with 2 lines:

EN-GB/PT-PT
FR-FR/PT-PT

This will create corpora for 4 pairs of languages: EN-PT, PT-EN and FR-PT and
PT-FR. A sample "LanguagePairs.txt" comes with the program to serve as an
example. Customise it to your needs respecting the syntax described above.

NOTE: Never erase the "LanguagePairs.txt" file and always make sure that each
pair of languages that you choose does exist in your TMX files. Otherwise, you
won't get any results.

The "Extract some corpora" and "Extract all corpora" functions are particularly
useful if you want to prepare corpora for several or many language pairs. If
your TMX files have translation units in all of the languages you are interested
in, put them in a single directory (it can have subdirectories) and use those
functions!

********************************************************************************
GETTING THE RESULTS:
********************************************************************************
The results are the aligned corpora files, as well as other files indicating how
well the processing was done.

When the processing is finished, you will find the corpora files in the
directory you have chosen when you selected "Select input/output directory". In
the "_processing_info" subdirectory of that directory you will find one or more
*errors.txt file(s), listing the name of the TMX files that caused an error, and
*actions.txt file(s), listing the files that were successfully processed as well
as the number of translation units extracted.

If you ask for the extraction of several corpora at once, you'll get lots of
corpora files. If you feel somewhat confused by that abundance, please note 2
things:
a) If you sort the files by order of modified date, you'll reconstitute the
chronological order in which the corpora were made (corpora are always made in
pairs one after the other);
b) The name of the corpora file has the following structure:

{Language of the segments} ({Language with which they are aligned})_{Base name
of the corpus}.txt
Example: the file "BG-01 (MT-01)_Corpus.txt" has segments in the BG-01
(Bulgarian) language that also have a translation in the MT-01 (Maltese)
language and corresponds to the corpus whose base name is "Corpus.txt". There
should be an equivalent "MT-01 (BG-01)_Corpus.txt", this time with all the
Maltese segments that have a translation in Bulgarian. Together, these 2 files
constitute an aligned corpus ready to be fed to Moses.

You can now feed Moses your customised corpora :-)

********************************************************************************
PERFORMANCE:
********************************************************************************
The program can process very large numbers of TMX files (tens of thousands or
more). It can also process extremely big TMX files (500 MB or more; it
successfully processed a 2,3 GB file). The extraction of the corpus of a pair of
languages in a very large (6,15 GB) set of TMX files took approximately 45
minutes in an Intel Core 2 Solo U3500 computer @ 1.4 GHz with 4 GB RAM.

The starting language and the destination language segments can be in any order
in the TMX files (e.g., the starting language segment may be found either before
or after the destination language segment in one, several or all translation
units of the TMX file).

The program accepts and preserves text in any language (including special
diacritical characters), but has only been tested with European Union official
languages.

********************************************************************************
THANKS:
********************************************************************************
Thanks to Gary Daine, who pointed out a way to improve one of the regex
expressions used in the code.

********************************************************************************
LICENSE:
********************************************************************************

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation (version 3 of the License).

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
