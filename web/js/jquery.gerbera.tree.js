/* global $ */

$.widget('grb.tree', {

  _create: function () {
    this.element.addClass('grb-tree')
    var data = this.options.data
    if (data.length > 0) {
      this.render(data, this.options.config)
    }
  },

  _destroy: function () {
    this.element.children('ul').remove()
    this.element.removeClass('grb-tree')
  },

  clear: function () {
    this.element.children('ul').remove()
  },

  render: function (data, config) {
    this.element.html('')
    this.generate(this.element, data, config)
  },

  generate: function (parent, data, config) {
    var items = []
    var newList = $('<ul></ul>')
    newList.addClass('list-group')

    for (var i = 0; i < data.length; i++) {
      var item = $('<li></li>')
      item.addClass('list-group-item')

      var icon = $('<span></span>').addClass(config.closedIcon).addClass('folder-icon')
      var title = $('<span></span>').addClass(config.titleClass).text(data[i].title)
      if (config.onSelection) {
        title.click(data[i], config.onSelection)
      }
      if (config.onExpand) {
        title.click(data[i], config.onExpand)
      }

      var badges = []
      var badgeData = data[i].badge
      if (badgeData && badgeData.length > 0) {
        for (var x = 0; x < badgeData.length; x++) {
          var aBadge = $('<span></span>').addClass('badge').text(badgeData[x])
          aBadge.addClass('pull-right')
          badges.push(aBadge)
        }
      }

      item.append(icon)
      item.append(title)
      item.append(badges)
      items.push(item)
      if (data[i].nodes && data[i].nodes.length > 0) {
        item.children('span').first().removeClass(config.closedIcon).addClass(config.openIcon)
        this.generate(item, data[i].nodes, config)
      }
    }
    newList.append(items)
    parent.append(newList)
  },

  append: function (parent, data) {
    parent.children('span').first().removeClass(this.options.config.closedIcon).addClass(this.options.config.openIcon)
    this.generate(parent, data, this.options.config)
  },

  closed: function (element) {
    return $(element).prev('span.folder-icon').hasClass(this.options.config.closedIcon)
  },

  collapse: function (element) {
    element.children('span').first().removeClass(this.options.config.openIcon).addClass(this.options.config.closedIcon)
    element.parent().find('ul.list-group').css('display', 'none')
  },

  select: function (element) {
    this.element.find('li').removeClass('selected-item')
    $(element).parent().addClass('selected-item')
  }

})
