sharedir = $(DESTDIR)@datadir@/@PACKAGE@

_DIST_FILES_IMAGES = \
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
./icons/nanotree/images/help_16x16.gif \
./icons/nanotree/images/t_no_root.gif \
./icons/nanotree/images/t.gif \
./icons/nanotree/images/plus_last.gif \
./icons/nanotree/images/lastnode.gif \
./icons/nanotree/images/plus_nolines.gif \
./icons/nanotree/images/document-new.png \
./icons/nanotree/images/folder-new.png \
./icons/nanotree/images/folder_closed.png \
./icons/nanotree/images/folder_open.png \
./icons/nanotree/images/autoscan_folder_closed.png \
./icons/nanotree/images/autoscan_folder_open.png \
./icons/nanotree/images/go-first.png \
./icons/nanotree/images/go-last.png \
./icons/nanotree/images/go-next.png \
./icons/nanotree/images/go-previous.png \
./icons/nanotree/images/remove_all.png \
./icons/nanotree/images/remove_this.png \
./icons/nanotree/images/stock-add.png \
./icons/nanotree/images/stock_edit.png \
./icons/nanotree/images/stock_exit.png \
./icons/nanotree/images/view-refresh.png

_DIST_FILES_JS = \
./js/autoscan.js \
./js/md5.js \
./js/prototype.js \
./js/auth.js \
./js/tree.js \
./js/tools.js \
./js/items.js \
./js/nanotree.js 

_DIST_FILES_WEB = \
./disabled.html \
./std_treelook.css \
./index.html \
./left.html \
./right.html \
./top.html \
./main.css \
./cm.xml \
./cds.xml \
./mr_reg.xml    

WEB_DIR=web
ICONS_DIR=$(WEB_DIR)/icons
NANOTREE_DIR=$(ICONS_DIR)/nanotree
IMAGES_DIR=$(NANOTREE_DIR)/images
JS_DIR=$(WEB_DIR)/js


EXTRA_DIST = $(_DIST_FILES_IMAGES) $(_DIST_FILES_JS) $(_DIST_FILES_WEB)

#iconsdir = $(sharedir)/web/icons/nanotree/images
#icons_DATA = $(_DIST_FILES_IMAGES)
#
#jsdir = $(sharedir)/web/js
#js_DATA = $(_DIST_FILES_JS)
#
#webdir = $(sharedir)/web
#web_DATA = $(_DIST_FILES_WEB)

install-data-local:
	$(mkinstalldirs) $(sharedir)
	$(mkinstalldirs) $(sharedir)/${WEB_DIR}
	$(mkinstalldirs) $(sharedir)/${ICONS_DIR}
	$(mkinstalldirs) $(sharedir)/${NANOTREE_DIR}
	$(mkinstalldirs) $(sharedir)/${IMAGES_DIR}
	$(mkinstalldirs) $(sharedir)/${JS_DIR}
	for i in $(_DIST_FILES_WEB); do \
      if test -f $$i; then \
        echo "$(INSTALL_DATA) $$i $(sharedir)/${WEB_DIR};"; \
        $(INSTALL_DATA) $$i $(sharedir)/${WEB_DIR}; \
      fi \
    done
	for i in $(_DIST_FILES_JS); do \
      if test -f $$i; then \
        echo "$(INSTALL_DATA) $$i $(sharedir)/${JS_DIR};"; \
        $(INSTALL_DATA) $$i $(sharedir)/${JS_DIR}; \
      fi \
    done
	for i in $(_DIST_FILES_IMAGES); do \
      if test -f $$i; then \
        echo "$(INSTALL_DATA) $$i $(sharedir)/${IMAGES_DIR};"; \
        $(INSTALL_DATA) $$i $(sharedir)/${IMAGES_DIR}; \
      fi \
    done
    

