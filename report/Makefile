# default target: compile the report pdf
all: report.pdf

PS4PDFDIR=./ps4pdf

export TEXINPUTS=:$(PS4PDFDIR)

# generic rule for using ps4pdf
# The idea is to run ps4pdf once and if the file contains some bibliography,
# run bibtex and recompile. If labels get changed etc., a third run is issued.
# The second and third runs are pdflatex only, because otherwise the .aux file
# gets re-created and forgets label positions.
%.pdf: %.tex preview.sty $(PS4PDFDIR)
	$(PS4PDFDIR)/ps4pdf $<
	if ( grep 'bibstyle' $*.aux >/dev/null 2>/dev/null ) ; then \
	  bibtex $*; \
	  pdflatex $<; \
	fi
	if ( \
	  grep 'ref{\|tableofcontents\|\\listof\|prosper' $*.tex >/dev/null 2>/dev/null \
	  || grep 'Rerun to get cross-references right.' $*.log >/dev/null 2>/dev/null \
	  || grep 'Citation.*undefined' $*.log >/dev/null 2>/dev/null \
	  ); then \
	  echo "## Reruning latex"; \
	  pdflatex $<; \
	fi


# "installing" preview.sty, installing the TeX way
preview.sty: preview.ins preview.dtx
	tex preview.ins


# download ps4pdf
ps4pdf.zip:
	wget http://theory.uwinnipeg.ca/scripts/CTAN/macros/latex/contrib/ps4pdf.zip

# unzip ps4pdf; for whatever reason, the ps4pdf script is not executable by default
$(PS4PDFDIR): ps4pdf.zip
	[ -d $@ ] || unzip $<
	chmod 755 $@/ps4pdf
