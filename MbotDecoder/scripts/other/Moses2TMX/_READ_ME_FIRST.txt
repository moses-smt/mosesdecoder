Summary:
	PURPOSE
	REQUIREMENTS
	INSTALLATION
	HOW TO USE
	LICENSE


********************************************************************************
PURPOSE:
********************************************************************************
This is the MS Windows and Linux version (tested with Ubuntu 10.10 and 11.04) of
Moses2TMX 1.033.

Moses2TMX started as a Windows program (tested on Windows7, Vista and XP) that
enables translators not necessarily with a deep knowledge of linguistic tools to
create TMX files from a Moses corpus or from any other corpus made up of 2
separate files, one for the source language and another for the target language,
whose lines are strictly aligned.

The program processes a whole directory containing source language and
corresponding target language documents and creates 1 TMX file (UTF-16 format;
Windows line endings) for each document pair that it processes.

The program accepts and preserves text in any language (including special
diacritical characters), but has only been tested with European Union official
languages.

The program is specifically intended to work with the output of a series of
Linux scripts together called Moses-for-Mere-Mortals.

********************************************************************************
REQUIREMENTS:
********************************************************************************
The program requires the following to be pre-installed in your computer:

1) Python 2.5 or higher (The program has been tested on Python 2.5 to 2.7)
	Windows users download and install from http://www.python.org/download/
	Ubuntu users can use the pre-installed Python distribution

2) wxPython 2.8 or higher
	Windows users download and install the Unicode version from
	http://www.wxpython.org/download.php
	Ubuntu users install with: sudo apt-get install python-wxtools

3) Pythoncard 0.8.2 or higher
	Windows users download and install
	http://sourceforge.net/projects/pythoncard/files/PythonCard/0.8.2/PythonCard-0.8.2.win32.exe/download
	Ubuntu users install with: sudo apt-get install pythoncard

********************************************************************************
INSTALLATION:
********************************************************************************
Windows users:
1) Download the Moses2TMX.exe file to a directory of your choice.
2) Double-click Moses2TMX.exe and follow the wizard's instructions.
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
1) Create a directory where you will copy the files that you want to process.

2) Copy the source and target language documents that you want to process to
that directory.
NOTE YOU HAVE TO RESPECT SOME NAMING CONVENTIONS IN ORDER TO BE ABLE TO USE
THIS PROGRAM:

	a) the target documents have to have follow the following convention:

		{basename}.{abbreviation of target language}.moses

		where {abbreviation of target language} is a ***2 character*** string
		containing the lowercased first 2 characters of any of the language
		codes present in the LanguageCodes.txt (present in the base directory of
		Moses2TMX)

		Example: If {basename} = "200000" and the target language has a code
		"EN-GB" in the LanguageCodes.txt, then the name of the target file
		should be "200000.en.moses"

	b) the source language document should have the name:

		{basename}

		Example: continuing the preceding example, the name of the corresponding
		source document should be "200000".

3) Launch the program as indicated above in the "Launching the program" section.

4) Operate on the main window of the program in the direction from top to
   bottom:
	a) Click the "Select Directory..." button to indicate the directory
	containing all the source and corresponding target documents that you want
	to process;
	b) Indicate the languages of your files refers to in the "Source Language"
	and "Target Language" comboboxes;
	c) Click the Create TMX Files button.

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
