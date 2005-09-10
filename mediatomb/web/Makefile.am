sharedir = $(DESTDIR)@datadir@/mediatomb

_DIST_FILES = \
./help.html \
./icons/file.png \
./icons/stock-add.png \
./icons/stock_default-folder.png \
./icons/stock-delete.png \
./icons/stock-edit.png \
./icons/stock_folder.png \
./icons/stock-go-back.png \
./icons/stock-go-forward.png \
./icons/stock-goto-first.png \
./icons/stock-goto-last.png \
./icons/stock-new.png \
./icons/stock-refresh.png \
./icons/stock_up-one-dir.png \
./index.html \
./js/prof.js \
./js/rollover.js \
./jssp/browse.jssp \
./jssp/browse_test.jssp \
./jssp/head.jssp \
./jssp/include \
./jssp/include/tools.jssp \
./jssp/index.jssp \
./jssp/items.jssp \
./jssp/login.jssp \
./jssp/tree.jssp \
./js/tools.js \
./js/tree.js \
./main.css \
./tree_icons \
./tree_icons/blank.gif \
./tree_icons/folder_closed.gif \
./tree_icons/folder_open.gif \
./tree_icons/join_both.gif \
./tree_icons/join_bottom.gif \
./tree_icons/join.gif \
./tree_icons/join_top.gif \
./tree_icons/minus_both.gif \
./tree_icons/minus_bottom.gif \
./tree_icons/minus_none.gif \
./tree_icons/minus_top.gif \
./tree_icons/plus_both.gif \
./tree_icons/plus_bottom.gif \
./tree_icons/plus_none.gif \
./tree_icons/plus_top.gif

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
    
