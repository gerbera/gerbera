
sharedir = $(DESTDIR)@datadir@/@PACKAGE@

_DIST_FILES = \
    config-dist.xml \
    mappings.xml \
    mysql.sql \
    sqlite3.sql

EXTRA_DIST = $(_DIST_FILES)

install-data-local:
	$(mkinstalldirs) $(sharedir)
	for i in $(_DIST_FILES); do \
	  if test -f $$i; then \
	    echo "$(INSTALL_DATA) $$i $(sharedir);"; \
	    $(INSTALL_DATA) $$i $(sharedir); \
	  fi \
	done

