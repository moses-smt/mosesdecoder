# default target: compile the report pdf
all: report.pdf

PS4PDFDIR=./ps4pdf

# generic rule for using ps4pdf
%.pdf: %.tex preview.sty $(PS4PDFDIR)
	$(PS4PDFDIR)/ps4pdf $<
	if ( grep 'bibstyle' $*.aux >/dev/null 2>/dev/null ) ; then \
	  bibtex $*; \
	  $(PS4PDFDIR)/ps4pdf $<; \
	fi
	if ( \
	  grep 'ref{\|tableofcontents\|\\listof\|prosper' $*.tex >/dev/null 2>/dev/null \
	  || grep 'Rerun to get cross-references right.' $*.log >/dev/null 2>/dev/null \
	  || grep 'Citation.*undefined' $*.log >/dev/null 2>/dev/null \
	  ); then \
	  echo "## Reruning latex"; \
	  $(PS4PDFDIR)/ps4pdf $<; \
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
