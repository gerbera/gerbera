SUBDIRS = icons

sharedir = $(DESTDIR)@datadir@/mediatomb

_DIST_FILES = \
	border.html \
	browse.xsl \
	cds.xml \
	cm.xml \
	defaults.xsl \
	didl.xsl \
	disabled.html \
	edit.xsl \
	error.xsl \
	head.html \
	help.html \
	index.html \
	login.html \
	main.css \
	navbar.html \
	new.xsl \
	noaccess.html \
	rollover.js \
	ui_config.xml

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
	$(mkinstalldirs) $(sharedir)/web/icons
	for i in icons/*.png; do \
	  if test -f $$i; then \
	    echo "$(INSTALL_DATA) $$i $(sharedir)/web/icons;"; \
	    $(INSTALL_DATA) $$i $(sharedir)/web/icons; \
	  fi \
	done

