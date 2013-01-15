#
# mkmakefile version 1.3.9 (C) 1995 by Kouichi ABE (WALL)
#
# Makefile for LaTeX
#

SRC = kanji.tex
DVI = $(SRC:.tex=.dvi)
PS  = $(SRC:.tex=.ps)

LATEX = platex
DVI2PS = dvi2ps
XDVI = xdvi
GS = gs

PRINT_COMMAND = lpr
PRINTER = rps

RM = /bin/rm -f

all:	$(PS)

$(PS):	$(DVI)
	$(DVI2PS) $(DVI) > $(PS)

$(DVI):	$(SRC)
	$(LATEX) $(SRC)
	$(LATEX) $(SRC)
	$(LATEX) $(SRC)

xview::	$(DVI)
	$(XDVI) $(DVI)

view::	$(PS)
	$(GS) $(PS)

print::	$(PS)
	$(PRINT_COMMAND) -P$(PRINTER) $(PS)

clean::
	-$(RM) *.log *.aux *.toc $(DVI) $(PS) *~ core *.core *.bak
