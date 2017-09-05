/* global $ GERBERA */

$.widget('grb.dataitems', {

  _create: function () {
    this.element.html('')
    this.element.addClass('grb-dataitems')
    var table = $('<table></table>').addClass('table')
    var tbody = $('<tbody></tbody>').addClass('table-striped table-hover').appendTo(table)
    var data = this.options.data
    var thead = $('<tr><th>Item</th></tr>')
    thead.appendTo(tbody)
    var row, content, text

    if (data.length > 0) {
      for (var i = 0; i < data.length; i++) {
        var item = data[i]
        row = $('<tr></tr>')
        content = $('<td></td>')

        if (item.url) {
          text = $('<a></a>')
          text.attr('href', item.url).text(item.text).appendTo(content)
        } else {
          text = $('<span></span>')
          text.text(item.text).appendTo(content)
        }

        if (item.img) {
          var img = $('<img/>')
          img.attr('src', item.img)
          img.addClass('pull-right')
          img.appendTo(content)
        }

        if (item.video) {
          var button = $('<button></button>')
          button.text('Play Video')
          button.addClass('pull-right')
          button.appendTo(content)
          button.click({item: item}, GERBERA.Items.playVideo)
        }

        row.addClass('grb-item')
        row.append(content)
        tbody.append(row)
      }
    } else {
      row = $('<tr></tr>')
      content = $('<td></td>')
      text = $('<span>No Items found</span>').appendTo(content)
      row.append(content)
      tbody.append(row)
    }

    this.element.append(table)
  },

  _destroy: function () {
    this.element.children('table').remove()
    this.element.removeClass('grb-dataitems')
  }

})
