# make            the whole handbook (handbook.pdf) and one theory.pdf per module
# make handbook   only handbook.pdf
# make ch07       only chapter 7 -> ../07-syscalls/theory.pdf
#
# pdflatex runs until the references are stable. Build files go to build/.

MODULES := 00-c-toolbox 01-processes 02-threads 03-race-conditions 04-locks \
           05-condvars-semaphores 06-deadlocks 07-syscalls 08-virtual-memory \
           09-scheduling 10-uthreads 11-sweb-bridge
CHAPTERS := $(foreach m,$(MODULES),ch$(firstword $(subst -, ,$(m))))

LATEX := pdflatex -interaction=nonstopmode -halt-on-error -output-directory build

all: handbook $(CHAPTERS)

handbook: handbook.pdf

handbook.pdf: handbook.tex preamble.tex $(addsuffix .tex,$(CHAPTERS))
	@mkdir -p build
	@for i in 1 2 3 4; do \
	  $(LATEX) handbook.tex > build/handbook.console || { tail -30 build/handbook.log; exit 1; }; \
	  grep -q "Rerun to get" build/handbook.log || break; \
	done
	cp build/handbook.pdf $@

# One chapter on its own, numbered as in the handbook.
$(CHAPTERS): ch%: ch%.tex preamble.tex
	@mkdir -p build
	@n=$$(echo $* | sed 's/^0*//'); n=$${n:-0}; \
	printf '%s\n' '\documentclass[11pt]{report}' '\input{preamble}' \
	  "\\setcounter{chapter}{$$((n - 1))}" '\begin{document}' '\input{ch$*}' \
	  '\end{document}' > build/theory-$*.tex
	@cd build && for i in 1 2 3 4; do \
	  TEXINPUTS=..: pdflatex -interaction=nonstopmode -halt-on-error theory-$*.tex > theory-$*.console \
	    || { tail -30 theory-$*.log; exit 1; }; \
	  grep -q "Rerun to get" theory-$*.log || break; \
	done
	cp build/theory-$*.pdf ../$(filter $*-%,$(MODULES))/theory.pdf

clean:
	rm -rf build

.PHONY: all handbook clean $(CHAPTERS)
