/*GRB*

    Gerbera - https://gerbera.io/

    jquery.gerbera.config.js - this file is part of Gerbera.

    Copyright (C) 2016-2025 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Gerbera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.

    $Id$
*/

class TreeItem {
  xpath = '';
  item = undefined;
  _value = undefined;
  config = undefined;
  parentItem = undefined;
  editor = undefined;
  pane = undefined;
  origValue = undefined;
  itemStatus = 'unchanged';
  source = '';
  _id = "-1";

  static userDocs = 'https://docs.gerbera.io/en/stable/';
  static RE = RegExp('[\\]/:[]', 'g');
  static LIST = 'grb_list_';
  static LINE = 'grb_line_';
  static ITEM = 'grb_item_';
  static VALUE = 'grb_value_';

  constructor(parentItem, metaList, item, values) {
    this.parentItem = parentItem;
    const meta = metaList.filter((m) => item.item === m.item);
    if (meta && meta.length > 0) {
      meta.forEach((m) => {
        this.config = m;
      });
    } else {
      const mapXPath = item.item ? item.item.replace(ParentTreeItem.idxReg, '') : item.item;
      values.filter((v) => v.item === item.item || v.item.replace(ParentTreeItem.idxReg, '') === mapXPath).forEach((v) => {
        metaList.filter((m) => Number.parseInt(v.aid) === Number.parseInt(m.id)).forEach((m) => {
          this.config = m;
        });
      });
    }
    this.xpath = item.item;

    const theValues = values.filter(v => v.item === this.xpath);
    if (theValues && theValues.length > 0) {
      this._value = theValues[0];
    } else {
      this._value = item;
    }
    this.source = 'source' in this._value ? this._value.source : 'default';
    this.itemStatus = (this._value && 'status' in this._value) ? this._value.status : this.itemStatus;
    this._id = (this._value && 'id' in this._value) ? this._value.id : this._id;
    this.item = item;
    this.origValue = this.value;

    this.setEntryChanged = this.setEntryChanged.bind(this);
    this.resetEntry = this.resetEntry.bind(this);
  }

  render(list, pane, level) {
    this.pane = pane;
    const line = $('<li></li>');
    line.attr('id', TreeItem.ITEM + this.item.item.replace(TreeItem.RE, '_'));
    list.removeClass('fa-ul');
    list.addClass('fa-ul');
    const itemLine = this.renderItemEditor(list, pane, level);
    this.renderDefaultInfo(itemLine);
  }

  addTextLine(line) {
    var text = $('<span></span>').addClass('configItemLabel');
    if (this.item.caption) {
      text.text(this.item.caption).appendTo(line);
      text.attr('title', this.item.item);
    } else {
      text.text(this.item.item).appendTo(line);
    }
  }

  addResultItem() {
    if (this.id && this.id !== '')
      this.pane.options.addResultItem(this.itemData);
  }

  setInputAttr(input) {
    input.hide();
  }

  setEntryChanged(event) {
    try {
      this.source = "ui";
      if (!this.itemStatus.match(/added/)) {
        $(this.editor[0].parentElement).removeClass(this.itemStatus.replace(/^.*,/,''));
        this.itemStatus = 'changed';
        $(this.editor[0].parentElement).addClass(this.itemStatus.replace(/^.*,/,''));
      }
      this.addResultItem();
    } catch (e) {
      console.error(event, e);
    }
  }

  resetValue() {
    this.editor[0].value = this.origValue;
  }

  resetEntry(event) {
    try {
      $(this.editor[0].parentElement).removeClass(this.itemStatus.replace(/^.*,/,''));
      this.itemStatus = 'reset';
      this.resetValue();
      $(this.editor[0].parentElement).addClass(this.itemStatus.replace(/^.*,/,''));
      this.addResultItem();
    } catch (e) {
      console.error(event, e);
    }
  }

  renderItemEditor(list, pane) {
    const itemLine = $('<li></li>').addClass('grb-config');
    itemLine.attr('id', TreeItem.LINE + this.xpath.replace(TreeItem.RE, '_'));
    list.append(itemLine);

    this.addTextLine(itemLine);

    var input = $('<input>', { "class": "configItemInput" });
    input.attr('id', TreeItem.VALUE + this.xpath.replace(TreeItem.RE, '_'));
    this.pane = pane;
    this.editor = input;

    input.attr('title', this._value ? this._value.item : this.xpath);

    itemLine.addClass(this.itemStatus.replace(/^.*,/,''));

    if (this.editable) {
      input.off('change').on('change', this.setEntryChanged);
    } else {
      input.prop('disabled', true);
    }
    this.setInputAttr(input, this.value);

    itemLine.append(input);
    if (this.help) {
      const link = $('<a>', { "title": "help", "target": "_blank", "style": "margin-left: 20px", "class": "", "href": TreeItem.userDocs + this.help });
      const icon = $('<i></i>', { "class": "fa " + "fa-info" });
      link.append(icon);
      link.appendTo(itemLine);
    }
    return itemLine;
  }

  renderItemButton() { }

  defaultText(helpLink) {
    if (this.defaultValue && this.defaultValue !== '') {
      if (!this.equals(this.defaultValue, this.value)) {
        helpLink.append(` default value is "${this.defaultValue}"`);
      } else {
        helpLink.append(' default value');
      }
    }
  }

  renderDefaultInfo(itemLine) {
    if (this.source === 'database') {
      const link = $('<a>', { "title": "reset", "style": "margin-left: 20px", "class": "", "href": "javascript:;" });
      const icon = $('<i></i>', { "class": "fa " + "fa-undo" });
      link.append(icon);
      if (this.origValue !== '') {
        link.append(` reset to ${this.origValue}`);
      } else {
        link.append(' reset');
      }
      this.defaultText(link);
      link.click(this, this.resetEntry);
      link.appendTo(itemLine);
    } else if (!this.itemStatus.match(/added/) && this.source === 'default') {
      const link = $('<a>', { "title": "copy", "style": "margin-left: 20px" });
      this.defaultText(link);
      link.appendTo(itemLine);
    } else if (!this.itemStatus.match(/unchanged/)) {
      const link = $('<a>', { "title": "reset", "style": "margin-left: 20px" });
      if (this.origValue !== '') {
        link.append(` config.xml value is "${this.origValue}"`);
      } else {
        link.append(' no config.xml entry');
      }
      this.defaultText(link);
      link.appendTo(itemLine);
    } else if (this.defaultValue && this.defaultValue !== '') {
      const link = $('<a>', { "title": "copy", "style": "margin-left: 20px" });
      this.defaultText(link);
      link.appendTo(itemLine);
    }
  }

  equals(firstValue, otherValue) {
    return firstValue === otherValue;
  }

  get uiValue() {
    return this.editor ? this.editor.val() : this.value;
  }

  get itemData() {
    return {
      item: this.xpath,
      id: this.id,
      value: this.uiValue,
      origValue: this.origValue,
      status: this.itemStatus
    };
  }

  get help() {
    if (this.item && 'help' in this.item && this.item.help && this.item.help.length > 0)
      return this.item.help;
    if (this.config && 'help' in this.config && this.config.help && this.config.help.length > 0)
      return this.config.help;
    return '';
  }

  get value() {
    if (this._value && 'value' in this._value && this._value.value != undefined && this._value.value != null)
      return this._value.value;
    if (this.item && 'value' in this.item && this.item.value != undefined && this.item.value != null)
      return this.item.value;
    if (this.config && 'value' in this.config && this.config.value != undefined && this.config.value != null)
      return this.config.value;
    return '';
  }

  get defaultValue() {
    return (this._value && 'defaultValue' in this._value) ? this._value.defaultValue : undefined;
  }

  get id() {
    if (this._id !== "-1")
      return this._id;
    if (this._value && 'id' in this._value && this._value.id != undefined && this._value.id != null)
      return this._value.id;
    if (this.config && 'id' in this.config && this.config.id != undefined && this.config.id != null)
      return this.config.id;
    return '-1';
  }

  get editable() {
    if (this.item && 'editable' in this.item)
      return this.item.editable;
    if (this.config && 'editable' in this.config)
      return this.config.editable;
    return true;
  }
}

class ParentTreeItem extends TreeItem {
  buttonRoot = undefined;
  children = [];
  static idxReg = RegExp('\\[[_0-9]+\\]', 'g');

  constructor(parentItem, meta, item, values, type = undefined) {
    super(parentItem, meta, item, values);
    if (type)
      this.item.type = type;
    if (item.children)
      this.handleChildren(meta, item.children, values);
  }

  addTextLine(line) {
    var count = this.editable ? this.children.length - 1 : this.children.length;
    var text = $('<span></span>').addClass('configItemLabel');
    var caption = '';
    if (this.item.caption) {
      caption = `${this.item.caption} (${this.item.item})`;
      text.attr('title', this.item.item);
    } else {
      caption = this.item.item;
    }

    if (count > 0) {
      caption += ` - ${count}`;
    } else {
      caption += ` - empty`;
    }

    text.text(caption).appendTo(line);
  }

  hideChild(event) {
    try {
      const myChild = $(this.parentElement).find('ul');
      if (myChild && myChild.length) {
        const icon = $(this.parentElement).find('i')[0];

        if (myChild[0].hidden) {
          myChild.show();
          $(icon).removeClass('fa-plus-square');
          $(icon).addClass('fa-minus-square');
          myChild[0].hidden = false;
        } else {
          myChild.hide();
          $(icon).removeClass('fa-minus-square');
          $(icon).addClass('fa-plus-square');
          myChild[0].hidden = true;
        }
      }
    } catch (e) {
      console.error(event, e);
    }
  }

  handleChildren(metaList, itemList, values) {
    for (var i = 0; i < itemList.length; i++) {
      const item = itemList[i];
      var type = item.type;
      const mapXPath = item.item.replace(ParentTreeItem.idxReg, '');

      if (type == undefined) {
        const meta = metaList.filter((m) => m.item === item.item || m.item.replace(ParentTreeItem.idxReg, '') === mapXPath);
        if (meta && meta.length > 0)
          type = meta[0].type;
      }

      if (type == undefined || type === 'Unset') {
        const itemMeta = [];
        values.filter((v) => v.item === item.item || v.item.replace(ParentTreeItem.idxReg, '') === mapXPath).forEach((v) => {
          metaList.filter((m) => Number.parseInt(m.id) === Number.parseInt(v.aid)).forEach((m) => {
            const metaItem = JSON.parse(JSON.stringify(m)); // create deep copy
            metaItem.item = item.item + '/' + m.item;
            itemMeta.push(metaItem);
            type = metaItem.type;
          });
        });
        metaList = metaList.concat(itemMeta);
      }

      item.type = type;

      switch (type) {
        case 'Boolean': {
          this.children.push(new CheckTreeItem(this, metaList, item, values));
          break;
        }
        case 'Password': {
          this.children.push(new PasswordTreeItem(this, metaList, item, values));
          break;
        }
        case 'Unset':
        case 'String':
        case 'Enum':
        case 'Path': {
          this.children.push(new TextTreeItem(this, metaList, item, values));
          break;
        }
        case 'Time':
        case 'Number': {
          this.children.push(new NumberTreeItem(this, metaList, item, values));
          break;
        }
        case 'Element': {
          this.children.push(new ElementTreeItem(this, metaList, item, values.filter((v) => v.item.startsWith(item.item))));
          break;
        }
        case 'List': {
          this.children.push(new ListTreeItem(this, metaList, item, item.item, values.filter((v) => v.item.startsWith(item.item))));
          break;
        }
        case 'SubList': {
          if (this.item.type !== 'List')
            this.children.push(new ListTreeItem(this, metaList, item, item.item, values));
          break;
        }
        default:
          console.warn(`Unknown item '${item.item}' with caption '${item.caption}' type '${item.type}'`);
      }
    }
  }

  renderChildren(list, pane, level) {
    this.children.forEach((c) => {
      c.render(list, pane, level + 1);
    });
  }

  renderTreeButtons() {
    this.children.forEach((c) => {
      c.renderItemButton();
    });
  }

  renderItemButton() {
    if (this.buttonRoot) {
      $(this.buttonRoot[0].children[0]).off('click').on('click', this.hideChild);
      $(this.buttonRoot[0].parentElement).addClass('fa-ul');
      const span = $('<span></span>').addClass('fa-li');
      $('<i class="fa"></i>').appendTo(span);
      span.off('click').on('click', this.hideChild);
      this.buttonRoot.prepend(span);

      $(this.buttonRoot[0].parentElement).find('ul').hidden = false;
      $(this.buttonRoot[0].children[0]).click();
    }
    this.renderTreeButtons();
  }

  addResultItem(inheritStatus = false) {
    this.children.forEach((child) => {
      if (inheritStatus) {
        child.itemStatus = this.itemStatus;
        child._id = child._id === '-1' ? this.id : child._id;
      }
      child.addResultItem();
    });
  }

  get itemData() { return undefined; }

  get itemDataRoot() { return super.itemData; }
}

class RootTreeItem extends ParentTreeItem {
  constructor(meta, item, values) {
    super(undefined, meta, item, values);
    this.parentItem = this;
  }

  render(list, pane, level) {
    this.renderChildren(list, pane, level);
    this.renderTreeButtons();
  }
}

class ElementTreeItem extends ParentTreeItem {
  constructor(parentItem, meta, item, values) {
    super(parentItem, meta, item, values);
  }

  render(list, pane, level) {
    this.pane = pane;
    const line = $('<li></li>');
    line.attr('id', TreeItem.LINE + this.xpath.replace(TreeItem.RE, '_'));
    line.addClass('configListItem');

    if (this.children.length > 0) {
      const subList = $('<ul></ul>').addClass('grb-config-element');
      subList.attr('id', TreeItem.LIST + this.item.item.replace(TreeItem.RE, '_'));
      this.addTextLine(line);
      this.renderChildren(subList, pane, level);
      subList.appendTo(line);
      this.buttonRoot = line;
    }
    line.addClass('grb-config');
    list.append(line);
  }
}

class ListTreeItem extends ParentTreeItem {
  itemCount = 0;
  constructor(parentItem, metaList, item, xpath, values) {
    super(parentItem, metaList, item, values);

    values.forEach(v => {
      if (v.item.startsWith(xpath)) {
        var entry = v.item.replace(xpath, '');
        const re = /^\[([0-9]+)\]+.*/;
        const reResult = re.exec(entry);
        if (reResult) {
          entry = reResult[1];
          const myVal = Number.parseInt(entry) + 1;
          this.itemCount = this.itemCount > myVal ? this.itemCount : myVal;
        } else if (entry.startsWith("[_]") && item.item == xpath + entry.replace("[_]", "")) {
          item.defaultValue = v.defaultValue;
          metaList.forEach((m) => {
            if (Number.parseInt(v.aid) === Number.parseInt(m.id)) {
              if (!('type' in item) || item.type == undefined || item.type === null || item.type === '') {
                item.type = m.type;
              }
              if (!('help' in item) || item.help == undefined || item.help === null || item.help === '') {
                item.help = m.help;
              }
            }
          });
        }
      }
    });

    this.children = []; // Children are for internal items only

    for (var count = 0; count < this.itemCount; count++) {
      const listItem = JSON.parse(JSON.stringify(item)); // create deep copy
      listItem.item = listItem.item + `[${count}]`;
      const meta = this.initListMeta(item, listItem, metaList);
      this.prepareListMeta(item, listItem, meta, metaList, values);
      const entry = new ListEntryTreeItem(this, metaList.concat(meta), listItem, values);
      if (entry.itemStatus.match(/killed/))
        continue;
      this.children.push(entry);
    }

    if (this.item.editable) {
      const addListItem = JSON.parse(JSON.stringify(item)); // create deep copy
      if (!addListItem.item.match(/\[_\]/))
        addListItem.item = addListItem.item + '[_]';
      const meta = this.initListMeta(item, addListItem, metaList);
      this.prepareListMeta(item, addListItem, meta, metaList, values);

      this.children.push(new AddEntryTreeItem(this, metaList.concat(meta), addListItem, values));
    }
  }

  render(list, pane, level) {
    this.pane = pane;
    const line = $('<li></li>');
    line.attr('id', TreeItem.LINE + this.item.item.replace(TreeItem.RE, '_'));
    line.addClass('configListItem');

    const subList = $('<ul></ul>').addClass('grb-config-list').addClass('fa-ul');
    subList.attr('id', TreeItem.LIST + this.item.item.replace(TreeItem.RE, '_'));
    this.addTextLine(line);
    this.renderChildren(subList, pane, level);
    subList.appendTo(line);
    this.buttonRoot = line;

    list.removeClass('fa-ul');
    list.addClass('fa-ul');
    if (!this.editable && this.children.length === 0) {
      const itemId = TreeItem.ITEM + this.xpath.replace(TreeItem.RE, '_') + "_new";
      var lineNew = $('<li></li>');
      lineNew.attr('id', TreeItem.LINE + this.item.item.replace(TreeItem.RE, '_') + "_new");
      var textNew = $('<span></span>');
      var symbolNew = $('<span></span>').addClass('fa-li');
      $('<i class="fa"></i>').addClass('fa-circle').appendTo(symbolNew);
      symbolNew.appendTo(lineNew);
      textNew.text('No Entries').appendTo(lineNew);
      var listNew = $('<ul></ul>');
      listNew.attr('id', itemId);
      lineNew.append(listNew);
      subList.append(lineNew);
    }
    line.addClass('grb-config');
    list.append(line);
    this.editor = subList;
  }

  initListMeta(item, listItem, metaList) {
    const meta = [];
    metaList.filter((m) => m.item.startsWith(item.item)).forEach((m) => {
      const metaItem = JSON.parse(JSON.stringify(m)); // create deep copy
      metaItem.item = metaItem.item.replace(item.item, listItem.item);
      meta.push(metaItem);
    });
    return meta;
  }

  prepareListMeta(item, listItem, meta, metaList, values) {
    if (!item || !item.children) return;

    listItem.children = [];
    item.children.forEach((subItem) => {
      const listEntry = JSON.parse(JSON.stringify(subItem)); // create deep copy
      listEntry.item = subItem.item.replace(item.item, listItem.item);
      listItem.children.push(listEntry);
      values.filter((v) => v.item.startsWith(listEntry.item)).forEach((v) => {
        metaList.filter((m) => { return Number.parseInt(m.id) === Number.parseInt(v.aid); }).forEach((m) => {
          const metaItem = JSON.parse(JSON.stringify(m)); // create deep copy
          metaItem.item = listEntry.item;
          if (!metaItem.type)
            metaItem.type = v.type;
          meta.push(metaItem);
        });
      });
      this.prepareListMeta(subItem, listEntry, meta, metaList, values); // recursive call
    });
  }
}

class ListEntryTreeItem extends ParentTreeItem {
  constructor(parentItem, meta, item, values) {
    super(parentItem, meta, item, values, 'ListEntry');
    if (this.children.length > 0) {
        this.itemStatus = this.children[0].itemStatus; // pull status up
        this._id = this.children[0]._id; // pull id up
    }
    this.removeItemClicked = this.removeItemClicked.bind(this);
    this.resetEntry = this.resetEntry.bind(this);
  }

  render(list, pane, level) {
    this.pane = pane;
    const subList = this.renderListEntry(list, pane, level);

    const itemLine = $('<li></li>');
    itemLine.attr('id', TreeItem.LINE + this.xpath.replace(TreeItem.RE, '_'));
    subList.append(itemLine);

    this.renderChildren(subList, pane, level);
    this.editor = $(subList[0]);
  }

  resetEntry(event) {
    if (this.parentItem.children.length > 1)
      super.resetEntry(event);
    else
      this.removeItemClicked(event);
  }

  renderListEntry(list, pane, level = 0) {
    const itemId = TreeItem.ITEM + this.xpath.replace(TreeItem.RE, '_');
    var itemList = list.find("#" + itemId);
    if (itemList === null || itemList.length === 0) {
      const line = $('<li></li>');
      var text = $(`<span>${this.item.item.replace(RegExp('^.*\\[([0-9]+)[^]]*$', 'g'), '$1')}</span>`).addClass('configItemLabel');
      text.appendTo(line);
      line.attr('id', TreeItem.LINE + this.item.item.replace(TreeItem.RE, '_'));
      list.removeClass('fa-ul');
      list.addClass('fa-ul');
      var symbol = $('<span></span>').addClass('fa-li');
      var icon = $('<i class="fa"></i>').addClass('fa-ban');
      icon.appendTo(symbol);
      symbol.appendTo(line);
      this.editor = line;
      this.editor.removeClass(this.itemStatus.replace(/^.*,/,'')).removeClass('unchanged').removeClass('default');
      if (this.editable) {
        if (this.itemStatus.match(/added/)) {
          icon.removeClass('fa-undo');
          icon.addClass('fa-ban');
        } else if (this.itemStatus.match(/removed/)) {
          icon.removeClass('fa-ban');
          icon.addClass('fa-undo');
        }
        this.editor.addClass(this.itemStatus.replace(/^.*,/,''));
        symbol.off('click').on('click', this.removeItemClicked);
      } else {
        icon.removeClass('fa-ban');
        icon.addClass('fa-circle');
      }
      itemList = $('<ul></ul>');
      itemList.removeClass('fa-ul');
      itemList.addClass('fa-ul');
      itemList.attr('id', itemId);
      line.append(itemList);
      itemList.addClass(this.itemStatus.replace(/^.*,/,''));
      if (level === -1) {
        line.insertBefore(list[0].lastChild);
      } else {
        list.append(line);
      }
    }
    return itemList;
  }

  removeItemClicked(event) {
    try {
      this.source = "ui";
      this.editor.removeClass(this.itemStatus.replace(/^.*,/,''));
      const icon = $(this.editor).find('i')[0];
      if (this.itemStatus.match(/removed/)) {
        this.itemStatus = 'reset';
        $(icon).removeClass('fa-undo');
        $(icon).addClass('fa-ban');
      } else if (this.itemStatus.match(/added/) || this.itemStatus.match(/manual/)) {
        this.editor[0].parentElement.hidden = true;
        this.itemStatus = this.itemStatus.replace(/added|manual/, 'killed');
        $(icon).removeClass('fa-ban');
        $(icon).addClass('fa-undo');
      } else {
        this.itemStatus = 'removed';
        $(icon).removeClass('fa-ban');
        $(icon).addClass('fa-undo');
      }

      this.addResultItem(true);
      this.editor.addClass(this.itemStatus.replace(/^.*,/,''));
    } catch (e) {
      console.error(event, e);
    }
  }
}

class AddEntryTreeItem extends ParentTreeItem {
  meta = [];
  values = [];

  constructor(parentItem, meta, item, values) {
    super(parentItem, meta, item, values);
    this.itemStatus = 'added';

    const mapXPath = this.xpath.replace(/\[_\]/, '');
    this.meta = meta.filter((m) => { return m.item.startsWith(this.xpath) || m.item.startsWith(mapXPath); });
    this.values = values.filter((v) => { return v.item.startsWith(this.xpath) || v.item.startsWith(mapXPath); });

    this.addItemClicked = this.addItemClicked.bind(this);
  }

  render(list, pane) {
    this.pane = pane;
    const itemId = TreeItem.ITEM + this.xpath.replace(TreeItem.RE, '_') + "_new";

    var line = $('<li></li>');
    line.attr('id', TreeItem.LINE + this.item.item.replace(TreeItem.RE, '_') + "_new");
    var text = $('<span></span>');
    var symbol = $('<span></span>').addClass('fa-li');
    $('<i class="fa fa-plus"></i>').appendTo(symbol);
    symbol.appendTo(line);
    symbol.off('click').on('click', this.addItemClicked);
    const caption = this.item.caption ? this.item.caption.replace(/ies$/, 'y').replace(/ces$/, 'ce').replace(/les$/, 'le').replace(/e?s$/, '') : 'Entry';
    text.text(`Add New ${caption}`).appendTo(line);
    var itemList = $('<ul></ul>');
    itemList.attr('id', itemId);
    line.append(itemList);
    list.append(line);
    this.editor = line;
  }

  addItemClicked(event) {
    try {
      this.source = "ui";
      const newItem = JSON.parse(JSON.stringify(this.item)); // deep copy
      const meta = JSON.parse(JSON.stringify(this.meta)); // deep copy
      meta.forEach((m) => {
        m.item = m.item.replace(/\[_\]/, `[${this.parentItem.itemCount}]`);
      });
      const values = JSON.parse(JSON.stringify(this.values)); // deep copy
      values.forEach((v) => {
        v.item = v.item.replace(/\[_\]/, `[${this.parentItem.itemCount}]`);
      });
      newItem.item = newItem.item.replace(/\[_\]/, `[${this.parentItem.itemCount}]`);
      newItem.children.forEach(child => {
        child.item = child.item.replace(/\[_\]/, `[${this.parentItem.itemCount}]`);
      });

      const treeItem = new ListEntryTreeItem(this.parentItem, this.meta, newItem, values);
      treeItem.itemStatus = 'added';
      this.parentItem.children.push(treeItem);
      treeItem.render(this.parentItem.editor, this.pane, -1);
      if (this.item.type === 'SubList') {
        var checkParent = this.parentItem;
        while (checkParent.item.type !== 'List' && checkParent !== checkParent.parentItem) {
            checkParent = checkParent.parentItem;
        }
        treeItem._id = treeItem._id === '-1' ? checkParent.id : treeItem._id;
        treeItem.itemStatus = 'changed,added';
      }
      treeItem.addResultItem(true);
      this.parentItem.itemCount++;
    } catch (e) {
      console.error(event, e);
    }
  }
}

class CheckTreeItem extends TreeItem {
  constructor(parentItem, meta, item, values) {
    super(parentItem, meta, item, values);
  }

  setInputAttr(input, value) {
    input.attr('type', 'checkbox');
    input.attr('checked', value === "true" || value === "on" || value === "yes");
  }

  get uiValue() {
    return (this.editor ? this.editor[0].checked : this.value) ? 'yes' : 'no';
  }

  resetValue() {
    this.editor[0].checked = this.origValue;
  }

  equals(firstValue, otherValue) {
    return (firstValue === true || firstValue === "true" || firstValue === "on" || firstValue === "yes") === (otherValue === true || otherValue === "true" || otherValue === "on" || otherValue === "yes");
  }
}

class NumberTreeItem extends TreeItem {
  constructor(parentItem, meta, item, values) {
    super(parentItem, meta, item, values);
  }

  setInputAttr(input, value) {
    input.attr('type', 'number');
    input.val(value);
  }

  equals(firstValue, otherValue) {
    return Number.parseFloat(firstValue) === Number.parseFloat(otherValue);
  }
}

class TextTreeItem extends TreeItem {
  constructor(parentItem, meta, item, values) {
    super(parentItem, meta, item, values);
  }

  setInputAttr(input, value) {
    input.attr('type', 'text');
    input.val(value);
  }
}

class PasswordTreeItem extends TreeItem {
  constructor(parentItem, meta, item, values) {
    super(parentItem, meta, item, values);
  }

  setInputAttr(input, value) {
    input.attr('type', 'password');
    input.val(value);
  }
}

$.widget('grb.config', {

  _create: function () {
    this.element.html('');
    this.element.addClass('grb-config');
    const list = $('<ul></ul>').addClass('grb-config-list');
    const chooser = this.options.chooser;
    this.setup = this.options.setup;
    this.meta = this.options.meta;
    if (this.options.userDocs && this.options.userDocs !== "")
      TreeItem.userDocs = this.options.userDocs;

    var line;
    this.result = {};
    this.configModeChanged = this.options.configModeChanged;

    const dbEntries = this.options.values.filter ((v) => v.source === 'database');
    const dbEntryCount = dbEntries.length;
    dbEntries.forEach((e) => {
        this.options.addDatabaseItem({
          item: e.item,
          id: e.id,
          value: e.value,
          origValue: e.value,
          status: e.item.split('[').length > 2 ? 'changed,killed' : 'killed'
        });
    });
    const cBox = $('<div></div>');
    cBox.attr('style', 'display: block; margin-top: -25px; position: absolute; right: 0px;');
    var tBox = $('<div></div>').addClass('small');
    tBox.append($(`<span style="margin-right: 20px"><strong>Database Entries:</strong>&nbsp;&nbsp;${dbEntryCount}</span>`));
    tBox.append($('<span style="margin-right: 10px"><strong>List Mode:</strong> </span>'));
    cBox.append(tBox);

    Object.getOwnPropertyNames(chooser).forEach((choice) => {
      var mBox = $('<div></div>').addClass('form-check').addClass('form-check-inline');
      var label = $('<label></label>').addClass('form-check-label');
      label.attr('for', 'configMode_' + choice);
      label.text(chooser[choice].caption + " ");
      var input = $('<input type="radio" name="configMode"></input>').addClass("form-check-input");
      input.attr('value', choice);
      input.attr('id', 'configMode_' + choice)
      var selection = {
        target: this
      };
      selection.configModeChanged = function (event) {
        selection.target.configModeChanged(choice, event);
      }

      input.off('change').on('change', selection.configModeChanged);
      label.append(input);
      mBox.append(label);
      tBox.append(mBox);
    });
    this.element.append(cBox);

    this.editable = false;
    const configMode = this.element.find('input[name=configMode]');
    configMode.val([this.options.choice]);
    if (this.setup.length > 0) {
      console.debug(this.meta);
      console.debug(this.setup);
      console.debug(this.options.values);
      var rootItem = { children: this.setup, item: '/' };
      const treeRoot = new RootTreeItem(this.meta, rootItem, this.options.values);
      treeRoot.render(list, this, 0);
    } else {
      line = $('<li></li>');
      $('<span>No config found</span>').appendTo(line);
      line.addClass('grb-config');
      list.append(line);
    }

    this.element.append(list);
    this.element.addClass('with-config');
  },

  _destroy: function () {
    this.element.children('table').remove();
    this.element.removeClass('grb-config');
    this.element.removeClass('with-config');
  },
});
