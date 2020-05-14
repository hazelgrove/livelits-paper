MAIN=livelits-paper

all:
	pdflatex -synctex=1 -interaction=nonstopmode $(MAIN)

bib:
	pdflatex $(MAIN)
	bibtex $(MAIN)
	pdflatex $(MAIN)
	bibtex $(MAIN)
	pdflatex -synctex=1 -interaction=nonstopmode $(MAIN)

clean: 
	rm -f *.aux *.log *.out $(MAIN).bbl main.blg main.pdf

