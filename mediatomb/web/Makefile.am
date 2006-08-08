sharedir = $(DESTDIR)@datadir@/mediatomb

_DIST_FILES = \
./help.html \
./index.html \
./js/prof.js \
./js/rollover.js \
./js/tools.js \
./js/tree.js \
./main.css 

EXTRA_DIST = $(_DIST_FILES)

install-data-local:
	$(mkinstalldirs) $(sharedir)
	$(mkinstalldirs) $(sharedir)/web
	for i in $(_DIST_FILES); do \
	  if test -f $$i; then \
	    echo "$(INSTALL_DATA) $$i $(sharedir)/web;"; \
	    $(INSTALL_DATA) $$i $(sharedir)/web; \
	  fi \
	done
    
