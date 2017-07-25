SHELL := /bin/sh
UNAME := $(shell uname)
.SUFFIXES:
.SUFFIXES: .pdf .tex .dvi

### Commands
#
# + pdf
#     create pdf from tex file.
#
# + punctuation
#     replace "。" and "、" with "．" and "，"
#
# + open
#     create pdf and open it.
#
# + clean
#     delete all files such as .aux, .log and .dvi that are normally created by running make.
#
# + distclean
#     delete all generated file (including .pdf).
#
# + rebuild
#     force to build a pdf
#
# + test
#     do test (requires ruby)
#

TEXENV_DIR := $(HOME)/.dotfiles/bash/texenvs

TEX := platex -recorder -shell-escape

TEX_FILE := $(wildcard *.tex)
PDF_FILE := $(patsubst %.tex, %.pdf, $(TEX_FILE))

COMPILE_CNT := 1

ifeq ($(UNAME), Linux)
OPEN := xdg-open
else
OPEN := open
endif

.PHONY: init pdf punctuation open clean distclean rebuild test

all: pdf

%.dvi: %.tex
	@for i in `seq 1 $(COMPILE_CNT)`; do \
		yes q | $(TEX) $<; \
	done
	@for i in `seq 1 3`; do \
		if grep -F 'Rerun to get cross-references right.' $(<:.tex=.log); then \
			yes q | $(TEX) $<; \
		else \
			exit 0; \
		fi; \
	done

%.pdf: %.dvi
	dvipdfmx -d5 $<

pdf: $(PDF_FILE)

punctuation punc pun: $(TEX_FILE)
	@$(foreach file, $?, \
		cat "$(file)" | sed -e 's/。/．/g' | sed -e 's/、/，/g' > tmp~ \
		&& mv tmp~ "$(file)"; \
	)

open: $(PDF_FILE)
	$(OPEN) $(PDF_FILE)

clean:
	$(RM) *.{aux,log,dvi,fls}

distclean: clean
	$(RM) $(PDF_FILE)

rebuild re:
	touch $(TEX_FILE)
	$(MAKE) pdf

test:
	latex-test $(TEX_FILE)
