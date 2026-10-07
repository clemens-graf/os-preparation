# make        -> ../guide.pdf
# make clean
LATEX := pdflatex -interaction=nonstopmode -halt-on-error -output-directory build
CHAPTERS := $(wildcard ch*.tex)

../guide.pdf: guide.tex preamble.tex $(CHAPTERS)
	@mkdir -p build
	@for i in 1 2 3 4; do \
	  $(LATEX) guide.tex > build/guide.console || { tail -40 build/guide.log; exit 1; }; \
	  grep -q "Rerun to get" build/guide.log || break; \
	done
	cp build/guide.pdf $@

clean:
	rm -rf build

.PHONY: clean
