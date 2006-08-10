sharedir = $(DESTDIR)@datadir@/mediatomb

_DIST_FILES = \
./icons/nanotree/images/minus.gif \
./icons/nanotree/images/plus.gif \
./icons/nanotree/images/minus_last_no_root.gif \
./icons/nanotree/images/minus_last.gif \
./icons/nanotree/images/white.gif \
./icons/nanotree/images/plus_no_root.gif \
./icons/nanotree/images/page16x16.gif \
./icons/nanotree/images/minus_nolines.gif \
./icons/nanotree/images/line.gif \
./icons/nanotree/images/user_16x16.gif \
./icons/nanotree/images/plus_last_no_root.gif \
./icons/nanotree/images/minus_no_root.gif \
./icons/nanotree/images/folder_open.gif \
./icons/nanotree/images/help_16x16.gif \
./icons/nanotree/images/t_no_root.gif \
./icons/nanotree/images/t.gif \
./icons/nanotree/images/folder_closed.gif \
./icons/nanotree/images/plus_last.gif \
./icons/nanotree/images/lastnode.gif \
./icons/nanotree/images/plus_nolines.gif \
./js/md5.js \
./js/prototype.js \
./js/auth.js \
./js/tree.js \
./js/tools.js \
./js/items.js \
./js/ajax.js \
./js/nanotree.js \
./std_treelook.css \
./index.html \
./help.html \
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
    
