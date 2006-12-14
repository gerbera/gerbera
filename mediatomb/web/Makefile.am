sharedir = $(DESTDIR)@datadir@/@PACKAGE@

_DIST_FILES_IMAGES = \
./icons/nanotree/images/minus.png \
./icons/nanotree/images/plus.png \
./icons/nanotree/images/minus_last_no_root.png \
./icons/nanotree/images/minus_last.png \
./icons/nanotree/images/white.png \
./icons/nanotree/images/plus_no_root.png \
./icons/nanotree/images/page16x16.gif \
./icons/nanotree/images/minus_nolines.png \
./icons/nanotree/images/line.png \
./icons/nanotree/images/user_16x16.gif \
./icons/nanotree/images/plus_last_no_root.png \
./icons/nanotree/images/minus_no_root.png \
./icons/nanotree/images/help_16x16.gif \
./icons/nanotree/images/t_no_root.png \
./icons/nanotree/images/t.png \
./icons/nanotree/images/plus_last.png \
./icons/nanotree/images/lastnode.png \
./icons/nanotree/images/plus_nolines.png \
./icons/nanotree/images/folder_closed.png \
./icons/nanotree/images/folder_open.png \
./icons/nanotree/images/autoscan_folder_closed.png \
./icons/nanotree/images/autoscan_folder_open.png

_DIST_FILES_ICONS = \
./icons/document-new.png \
./icons/folder_new.png \
./icons/go-first.png \
./icons/go-last.png \
./icons/go-next.png \
./icons/go-previous.png \
./icons/remove_all.png \
./icons/remove_this.png \
./icons/stock-add.png \
./icons/add_as_autoscan.png \
./icons/stock_edit.png \
./icons/stock_exit.png \
./icons/view-refresh.png

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

#WEB_DIR=web
#ICONS_DIR=$(WEB_DIR)/icons
#NANOTREE_DIR=$(ICONS_DIR)/nanotree
#IMAGES_DIR=$(NANOTREE_DIR)/images
#JS_DIR=$(WEB_DIR)/js


EXTRA_DIST = $(_DIST_FILES_IMAGES) $(_DIST_FILES_JS) $(_DIST_FILES_WEB) \
             $(_DIST_FILES_ICONS)

imagesdir = $(sharedir)/web/icons/nanotree/images
images_DATA = $(_DIST_FILES_IMAGES)

iconsdir = $(sharedir)/web/icons
icons_DATA = $(_DIST_FILES_ICONS)
    
jsdir = $(sharedir)/web/js
js_DATA = $(_DIST_FILES_JS)

webdir = $(sharedir)/web
web_DATA = $(_DIST_FILES_WEB)

#install-data-local:
#	$(mkinstalldirs) $(sharedir)
#	$(mkinstalldirs) $(sharedir)/${WEB_DIR}
#	$(mkinstalldirs) $(sharedir)/${ICONS_DIR}
#	$(mkinstalldirs) $(sharedir)/${NANOTREE_DIR}
#	$(mkinstalldirs) $(sharedir)/${IMAGES_DIR}
#	$(mkinstalldirs) $(sharedir)/${JS_DIR}
#	for i in $(_DIST_FILES_WEB); do \
#      if test -f $$i; then \
#        echo "$(INSTALL_DATA) $$i $(sharedir)/${WEB_DIR};"; \
#        $(INSTALL_DATA) $$i $(sharedir)/${WEB_DIR}; \
#      fi \
#    done
#	for i in $(_DIST_FILES_JS); do \
#      if test -f $$i; then \
#        echo "$(INSTALL_DATA) $$i $(sharedir)/${JS_DIR};"; \
#        $(INSTALL_DATA) $$i $(sharedir)/${JS_DIR}; \
#      fi \
#    done
#	for i in $(_DIST_FILES_IMAGES); do \
#      if test -f $$i; then \
#        echo "$(INSTALL_DATA) $$i $(sharedir)/${IMAGES_DIR};"; \
#        $(INSTALL_DATA) $$i $(sharedir)/${IMAGES_DIR}; \
#      fi \
#    done
    

