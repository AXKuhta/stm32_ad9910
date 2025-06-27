
all: document.pdf

document.pdf: users_manual.md
	@pandoc "users_manual.md" -o "document.pdf" --from markdown --template ./Eisvogel-3.2.0/eisvogel.latex --listings
