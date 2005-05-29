sharedir = $(DESTDIR)@datadir@/mediatomb

JS_DIR = js

_DIST_FILES = \
	import.js

    
EXTRA_DIST = $(_DIST_FILES)

install-data-local:
	$(mkinstalldirs) $(sharedir)
	$(mkinstalldirs) $(sharedir)/${JS_DIR}
	for i in $(_DIST_FILES); do \
	  if test -f $$i; then \
	    echo "$(INSTALL_DATA) $$i $(sharedir)/${JS_DIR};"; \
	    $(INSTALL_DATA) $$i $(sharedir)/${JS_DIR}; \
	  fi \
	done

