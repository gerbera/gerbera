/*GRB*

    Gerbera - https://gerbera.io/

    jquery.gerbera.config.js - this file is part of Gerbera.

    Copyright (C) 2016-2020 Gerbera Contributors

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
$.widget('grb.config', {

  _create: function () {
    this.element.html('');
    this.element.addClass('grb-config');
    const list = $('<ul></ul>').addClass('list');
    const data = this.options.meta;
    let line;
    this.subElements = [];
    this.result = {};

    console.log(this.options);

    if (data.length > 0) {
      this.createSection(list, '', data, this.options.values, 0, this);
    } else {
      line = $('<li></li>');
      $('<span>No config found</span>').appendTo(line);
      line.addClass('grb-config');
      list.append(line);
    }

    this.subElements.forEach((elm) => {
      if (elm.item.hasClass('configListItem')) {
        $(elm.item[0].children[0]).off('click').on('click', this.hideChild);

        $(elm.item[0].parentElement).addClass('fa-ul');
        const span = $('<span></span>').addClass('fa-li');
        $('<i class="fa"></i>').appendTo(span);
        span.off('click').on('click', this.hideChild);
        elm.item.prepend(span);

        $(elm.item[0].parentElement).find('ul').hidden = false;
        $(elm.item[0].children[0]).click();
      }
    });

    this.element.append(list);
    this.element.addClass('with-config');
  },

  _destroy: function () {
    this.element.children('table').remove();
    this.element.removeClass('grb-config');
    this.element.removeClass('with-config');
  },

  hideChild: function () {
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
      console.log(e);
    }
  },

  resetEntry: function (itemValue) {
console.log({resetEntry: itemValue});
    this.result[itemValue.item] = itemValue;
    this.options.addResultItem(itemValue);
    itemValue.value = itemValue.editor.val();
    switch(itemValue.config.type){
      case 'Boolean': {
        itemValue.value = itemValue.editor[0].checked;
        break;
      }
      case 'String':
      case 'Password':
      case 'Number': {
        itemValue.value = itemValue.editor.val();
        break;
      }
      case 'Element':
      case 'List':
        // nothing to do
      break;
      default:
        console.log('Unknown item type ' + itemValue.config.type);
    }
    $(itemValue.editor[0].parentElement).removeClass(itemValue.status);
    itemValue.status = 'reset';
    $(itemValue.editor[0].parentElement).addClass(itemValue.status);
  },

  setEntryChanged: function (itemValue) {
console.log({setEntryChanged: itemValue});
    this.result[itemValue.item] = itemValue;
    this.options.addResultItem(itemValue);
    itemValue.origValue = itemValue.source === "config.xml" ? itemValue.value : itemValue.origValue;
    itemValue.source = "ui";
    itemValue.value = itemValue.editor.val();
    switch(itemValue.config.type){
      case 'Boolean': {
        itemValue.value = itemValue.editor[0].checked;
        break;
      }
      case 'String':
      case 'Password':
      case 'Number': {
        itemValue.value = itemValue.editor.val();
        break;
      }
      case 'Element':
      case 'List':
        // nothing to do
      break;
      default:
        console.log('Unknown item type ' + itemValue.config.type);
    }
    $(itemValue.editor[0].parentElement).removeClass(itemValue.status);
    itemValue.status = 'changed';
    $(itemValue.editor[0].parentElement).addClass(itemValue.status);
  },

  removeItemClicked: function (listValue) {
console.log({removeItemClicked: listValue});
    this.result[listValue.item] = listValue;
    this.options.addResultItem(listValue);
    listValue.source = "ui";
    listValue.editor.removeClass(listValue.status);
    const icon = $(listValue.editor).find('i')[0];
    if (listValue.status === 'removed') {
        listValue.status = 'reset';
        $(icon).removeClass('fa-undo');
        $(icon).addClass('fa-ban');
    } else if (listValue.status === 'added' || listValue.status === 'manual') {
        listValue.editor[0].hidden = true;
        listValue.status = 'killed';
        $(icon).removeClass('fa-ban');
        $(icon).addClass('fa-undo');
    } else {
        listValue.status = 'removed';
        $(icon).removeClass('fa-ban');
        $(icon).addClass('fa-undo');
    }

    const changedChildren = (listValue.parentItem.childList !== undefined) ? Object.getOwnPropertyNames(listValue.parentItem.childList) : [];
    if (changedChildren.length > 0) {
      changedChildren.forEach((child) => {
        let childValue = listValue.parentItem.childList[child];
        if (childValue &&  childValue.length > 0) {
          childValue.forEach((j) => {
            j.status = listValue.status;
            this.options.addResultItem(j);
          });
        }
      });
    }
    listValue.editor.addClass(listValue.status);
  },

  addItemClicked: function (listValue) {
console.log({addItemClicked: listValue});
    this.result[listValue.item] = listValue;
    this.options.addResultItem(listValue);

    listValue.source = "ui";
    if (listValue.parentItem.children.length > 0) {
      let values = [];
      listValue.parentItem.children.forEach(child => {
        const newValue = {
          config: child,
          id: "-1",
          item: listValue.parentItem.item + `[${listValue.index}]` + child.item.replace(listValue.parentItem.item, ''),
          status: 'added',
          value: child.value ? child.value : '',
          source: 'ui',
          origValue: ''
         };
        if (child.editable) {
          this.options.addResultItem(newValue);
        }
        values.push(newValue);
      });
      this.createSection($(listValue.editor[0].parentElement), listValue.parentItem.item, listValue.parentItem.children, values, -1, listValue.parentItem, 'added');
      this.subElements.push({item: listValue.editor, child: listValue.editor[0].parentElement});
    }
    listValue.index++;
  },

  createSection: function (list, xpath, section, values, level, parentItem, status = 'unchanged') {
    parentItem.childList = {};
    parentItem.childList[0] = [];
    for (let i = 0; i < section.length; i++) {
      const item = section[i];
      let line;

      if (item.type === 'Element') {
        line = $('<li></li>');
        line.attr('id', "line_" + item.item.replaceAll("/","_"));
        line.addClass('configListItem');
        // recursive call
        let subList = null;
        if (item.children.length > 0) {
          subList = $('<ul></ul>').addClass('element');
          subList.attr('id', "list_" + item.item.replaceAll("/","_"));
          this.addTextLine(line,item);
          this.createSection (subList, xpath, item.children, values, level+1, item);
          subList.appendTo(line);
          this.subElements.push({item: line, child: subList});
        }
        line.addClass('grb-config');
        list.append(line);
      } else if (item.type === 'List') {
        line = $('<li></li>');
        line.attr('id', "line_" + item.item.replaceAll("/","_"));
        line.addClass('configListItem');
        // recursive call
        if (item.children.length > 0) {
          const subList = $('<ul></ul>').addClass('list');
          subList.attr('id', "list_" + item.item.replaceAll("/","_"));
          this.addTextLine(line, item);
          this.createSection (subList, item.item, item.children, values, level+1, item);
          subList.appendTo(line);
          this.subElements.push({item: line, child: subList});
        }
      } else {
        let itemCount = -1;

        if (xpath && xpath.length > 0) {
          line = $('<li></li>');
          line.attr('id', "line_" + item.item.replaceAll("/","_"));
          values.forEach(v => {
            if (v.item.startsWith(xpath)) {
              var entry = v.item.replace(xpath, '');
              const re = /^\[([0-9]+)\]+.*/;
              const result = re.exec(entry);
              if (result) {
                entry = result[1];
                const myVal = Number.parseInt(entry) + 1;
                itemCount = itemCount>myVal ? itemCount : myVal;
              }
            }
          });
        } else {
          itemCount = 1;
          line = $('<li></li>');
          line.attr('id', "item_" + item.item.replaceAll("/","_"));
        }
        if (itemCount < 1 && list[0].childElementCount === 0 ) {
          list.removeClass('fa-ul');
          list.addClass('fa-ul');
          const itemId = "item_" + xpath.replaceAll("/","_") + "_new";
          if (parentItem.editable) {
            this.addNewListItemBlock(list, item, parentItem, 0, xpath, itemId);
          } else {
            let lineNew = $('<li></li>');
            lineNew.attr('id', "line_" + item.item.replaceAll("/","_"));
            let textNew = $('<span></span>');
            let symbolNew = $('<span></span>').addClass('fa-li');
            $('<i class="fa"></i>').addClass('fa-circle').appendTo(symbolNew);
            symbolNew.appendTo(lineNew);
            textNew.text('No Entries').appendTo(lineNew);
            let xpathListNew = $('<ul></ul>');
            xpathListNew.attr('id', itemId + '_New');
            lineNew.append(xpathListNew);
            list.append(lineNew);
          }
        }
        let startCount = 0;
        if (level === -1 && itemCount > 0) {
          startCount = itemCount - 1;
        }

        for (let count = startCount; count < itemCount; count++) {
          let itemLine = line;
          let lineStatus = status;

          if (xpath && xpath.length > 0) {
            const itemId = "item_" + xpath.replaceAll("/","_") + "_" + count
            let xpathList = list.find("#" + itemId);
            if (xpathList === null || xpathList.length === 0) {
              line = $('<li></li>');
              line.attr('style', 'padding-top: 20px');
              line.attr('id', "line_" + item.item.replaceAll("/","_"));
              list.removeClass('fa-ul');
              list.addClass('fa-ul');
              let symbol = $('<span></span>').addClass('fa-li');
              let icon = $('<i class="fa"></i>').addClass('fa-ban');
              icon.appendTo(symbol);
              symbol.appendTo(line);
              const listValue = {
                item: xpath + `[${count}]`,
                index: count,
                target: this,
                editor: line,
                status: lineStatus,
                parentItem: parentItem,
                editable: parentItem.editable};
              parentItem.childList[count] = [];
              values.filter((v) => { return v.item == listValue.item; }).forEach((v) => {
                listValue.status = v.status;
                listValue.id = v.id;
              });
              listValue.editor.removeClass(lineStatus);
              lineStatus = listValue.status;
              if (listValue.editable) {
                if (listValue.status === 'added') {
                    icon.removeClass('fa-undo');
                    icon.addClass('fa-ban');
                } else if (listValue.status === 'removed') {
                    icon.removeClass('fa-ban');
                    icon.addClass('fa-undo');
                }
                listValue.editor.addClass(listValue.status);
                listValue.removeItemClicked = function(event) {
                  listValue.target.removeItemClicked (listValue, event);
                }
                symbol.off('click').on('click', listValue.removeItemClicked);
              } else {
                icon.removeClass('fa-ban');
                icon.addClass('fa-circle');
              }
              //let text = $('<span></span>');
              //text.text(count).appendTo(line);
              //text.attr('title', item.item);
              xpathList = $('<ul></ul>');
              xpathList.attr('id', itemId);
              line.append(xpathList);
              xpathList.addClass(listValue.status);
              if (level === -1) {
                line.insertBefore(list[0].lastChild);
              } else {
                list.append(line);
              }
              if (count === itemCount - 1 && level !== -1 && listValue.editable) {
                  this.addNewListItemBlock(list, item, parentItem, itemCount, xpath, itemId);
              }
            }
            itemLine =  $('<li></li>');
            itemLine.attr('id', "line_" + xpath.replaceAll("/","_") + "_" + count);
            xpathList.append(itemLine);
          }

          this.addTextLine(itemLine, item);

          let input = $('<input>');
          input.attr('id', "value_" + item.item.replaceAll("/","_") + "_" + i +  "_" + count);
          input.attr('style', 'margin-left: 20px; min-width: 400px');
          let itemValue = {value: item.value, source: 'default', status: lineStatus};

          values.forEach(v => {
            if (v.item === item.item) {
              itemValue = v;
              input.attr('title', item.item);
            } else if (xpath && xpath.length > 0) {
              if (xpath + `[${count}]` +  item.item.replace(xpath,'') === v.item) {
                //console.log(xpath + `[${count}]` +  item.item.replace(xpath,''));
                itemValue = v;
                input.attr('title', v.item);
              }
            }
          });
          parentItem.childList[count].push(itemValue);
          itemValue.status = lineStatus;
          itemLine.addClass(itemValue.status);

          if(item.editable) {
            itemValue.setEntryChanged = function (event) {
              itemValue.target.setEntryChanged(itemValue, event);
            }
            itemValue.target = this;
            itemValue.config = item;
            itemValue.editor = input;
            input.off('change').on('change', itemValue.setEntryChanged);
          } else {
            input.attr('disabled', '');
          }

          switch(item.type){
            case 'Boolean': {
              input.attr('type', 'checkbox');
              input.attr('checked', itemValue.value === "true");
            }
            break;
            case 'String': {
              input.attr('type', 'text');
              input.val(itemValue.value);
            }
            break;
            case 'Password': {
              input.attr('type', 'password');
              input.val(itemValue.value);
            }
            break;
            case 'Comment': {
              input.hide();
            }
            break;
            case 'Number': {
              input.attr('type', 'number');
              input.val(itemValue.value);
            }
            break;
            case 'Element':
            case 'List':
              // already handled
            break;
            default:
              console.log('Unknown item type ' + item.type);
          }

          itemLine.append(input);
          if (itemValue.source === 'database') {
            const link = $('<a>', {"title": "reset", "style": "margin-left: 20px", "class": "", "href": "javascript:;"});
            const icon = $('<i></i>', {"class": "fa " + "fa-undo" });
            link.append(icon);
            if (itemValue.origValue !== '') {
                link.append(` reset to ${itemValue.origValue}`);
            } else {
                link.append(' reset');
            }
            itemValue.resetEntry = function (event) { itemValue.target.resetEntry(itemValue, event); }
            link.click(itemValue, itemValue.resetEntry);
            link.appendTo(itemLine);
          } else if (itemValue.status !== 'unchanged') {
            const link = $('<a>', {"title": "reset", "style": "margin-left: 20px"});
            if (itemValue.origValue !== '') {
                link.append(` config.xml value is "${itemValue.origValue}"`);
            } else {
                link.append(' no config.xml entry');
            }
            link.appendTo(itemLine);
          }
        }
      }

      if (!xpath || xpath.length === 0) {
        line.addClass('grb-config');
        list.append(line);
      }
    }
  },

  addNewListItemBlock: function (list, item, parentItem, itemCount, xpath, itemId) {
    let lineNew = $('<li></li>');
    lineNew.attr('id', "line_" + item.item.replaceAll("/","_"));
    let textNew = $('<span></span>');
    let symbolNew = $('<span></span>').addClass('fa-li');
    $('<i class="fa"></i>').addClass('fa-plus').appendTo(symbolNew);
    symbolNew.appendTo(lineNew);
    const listValueNew = {
      item: xpath + `[${itemCount}]`,
      index: itemCount,
      target: this,
      editor: lineNew,
      status: 'added',
      parentItem: parentItem };
    listValueNew.addItemClicked = function(event) {
      listValueNew.target.addItemClicked (listValueNew, event);
    }
    symbolNew.off('click').on('click', listValueNew.addItemClicked);
    textNew.text('Add New Entry').appendTo(lineNew);
    //textNew.attr('title', item.item);
    let xpathListNew = $('<ul></ul>');
    xpathListNew.attr('id', itemId + '_New');
    lineNew.append(xpathListNew);
    list.append(lineNew);
  },

  addTextLine: function (line, item) {
    let text = $('<span></span>');
    if (item.caption && item.children && item.children.length > 0) {
      text.text(`${item.caption} (${item.item})`).appendTo(line);
    } else if (item.caption) {
      text.text(item.caption).appendTo(line);
      text.attr('style', 'min-width: 200px; display: inline-block;');
      text.attr('title', item.item);
    } else {
      text.text(item.item).appendTo(line);
    }
  }
});
