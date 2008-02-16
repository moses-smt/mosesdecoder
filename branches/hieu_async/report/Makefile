# default target: compile the report pdf
all: report.pdf

PS4PDFDIR=./ps4pdf

#useful for anything other than a standard linux system
#TEXDIR = c:/progra~1/miktex/miktex-2.4/miktex/bin
#TEX = $(TEXDIR)/tex
#PDFLATEX = $(TEXDIR)/pdflatex
#BIBTEX = $(TEXDIR)/bibtex
TEX = tex
PDFLATEX = pdflatex
BIBTEX = bibtex

export TEXINPUTS=:$(PS4PDFDIR)

# These packages are needed to compile e.g. on login.cslp, though they might
# be present in your TeX installations in versions recent enough.
EXTRA_PACKAGES=preview.sty caption.sty subfig.sty

# generic rule for using ps4pdf
# The idea is to run ps4pdf once and if the file contains some bibliography,
# run bibtex and recompile. If labels get changed etc., a third run is issued.
# The second and third runs are pdflatex only, because otherwise the .aux file
# gets re-created and forgets label positions.
%.pdf: %.tex $(EXTRA_PACKAGES) $(PS4PDFDIR)
	$(PS4PDFDIR)/ps4pdf $<
	if ( grep 'bibstyle' $*.aux >/dev/null 2>/dev/null ) ; then \
	  $(BIBTEX) $*; \
	  $(PDFLATEX) $<; \
	fi
	if ( \
	  grep 'ref{\|tableofcontents\|\\listof\|prosper' $*.tex >/dev/null 2>/dev/null \
	  || grep 'Rerun to get cross-references right.' $*.log >/dev/null 2>/dev/null \
	  || grep 'Citation.*undefined' $*.log >/dev/null 2>/dev/null \
	  ); then \
	  echo "## Reruning latex"; \
	  $(PDFLATEX) $<; \
	fi

# additional dependence, just make sure to install these, too
caption.dtx: caption2.dtx

# "installing" preview.sty, caption.sty or subfig.sty, installing the TeX way
%.sty: %.ins %.dtx
	$(TEX) $<


# download ps4pdf
ps4pdf.zip:
	wget http://theory.uwinnipeg.ca/scripts/CTAN/macros/latex/contrib/ps4pdf.zip

# unzip ps4pdf; for whatever reason, the ps4pdf script is not executable by default
$(PS4PDFDIR): ps4pdf.zip
	[ -d $@ ] || unzip $<
	chmod 755 $@/ps4pdf
