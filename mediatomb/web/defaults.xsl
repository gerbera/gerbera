<xsl:transform version="1.0"
               xmlns:dc="http://purl.org/dc/elements/1.1/"
               xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
               xmlns:upnp="urn:schemas-upnp-org:metadata-1-0/upnp/">

    <xsl:variable name="ui_config" select="document('ui_config.xml')"/>
               
    <!-- background color 1 --> 
    <xsl:variable name="bgcolor_1">
        <xsl:value-of select="$ui_config/ui/colors/color1"/>
    </xsl:variable>

    <!-- background color 2 -->
    <xsl:variable name="bgcolor_2">
        <xsl:value-of select="$ui_config/ui/colors/color2"/>
    </xsl:variable>
    
    <!-- width of table cell for one icon -->
    <xsl:variable name="icon_cell_width"> 
        <xsl:value-of select="$ui_config/ui/icons/attribute::cell_width"/>
    </xsl:variable>

    <!-- heigt of table cell for one icon -->
    <xsl:variable name="icon_cell_height">
        <xsl:value-of select="$ui_config/ui/icons/attribute::cell_height"/>
    </xsl:variable>

    <!-- path (relative to server webroot) where all icons are located -->
    <xsl:variable name="icons_path">
        <xsl:value-of select="$ui_config/ui/icons/attribute::path"/>
    </xsl:variable>

    <!-- default icon to go up one level -->
    <xsl:variable name="icon_directory_up">
        <xsl:call-template name="get_icon">
            <xsl:with-param name="name" select="'dir_up'"/>
        </xsl:call-template>
    </xsl:variable> 

    <!-- default container icon-->
    <xsl:variable name="icon_container">
        <xsl:call-template name="get_icon">
            <xsl:with-param name="name" select="'container'"/>
        </xsl:call-template>
    </xsl:variable>

    <!-- default item icon -->
    <xsl:variable name="icon_item">
        <xsl:call-template name="get_icon">
            <xsl:with-param name="name" select="'file'"/>
        </xsl:call-template>
    </xsl:variable>

    <!-- default icon for "top of the list" -->
    <xsl:variable name="icon_top_list">
        <xsl:call-template name="get_icon">
            <xsl:with-param name="name" select="'first_page'"/>
        </xsl:call-template>
    </xsl:variable>

    <!-- default icon for "bottom of the list" -->
    <xsl:variable name="icon_bottom_list">
        <xsl:call-template name="get_icon">
            <xsl:with-param name="name" select="'last_page'"/>
        </xsl:call-template>
    </xsl:variable>

    <!-- default icon for "bottom of the list" -->
    <xsl:variable name="icon_current">
        <xsl:call-template name="get_icon">
            <xsl:with-param name="name" select="'current_container'"/>
        </xsl:call-template>
    </xsl:variable>

    <!-- default icon for "one page up" -->
    <xsl:variable name="icon_page_up">
        <xsl:call-template name="get_icon">
            <xsl:with-param name="name" select="'previous_page'"/>
        </xsl:call-template>
    </xsl:variable>
    
    <!-- default icon for "one page down" -->
    <xsl:variable name="icon_page_down">
        <xsl:call-template name="get_icon">
            <xsl:with-param name="name" select="'next_page'"/>
        </xsl:call-template>
    </xsl:variable>

    <!-- default icon for "add folder/item" -->
    <xsl:variable name="icon_add">
        <xsl:call-template name="get_icon">
            <xsl:with-param name="name" select="'add'"/>
        </xsl:call-template>
    </xsl:variable>

    <xsl:template name="get_icon">
        <xsl:param name="name" select="''"/>
        <xsl:for-each select="$ui_config/ui/icons/icon">
            <xsl:if test="attribute::type = $name">
                <xsl:value-of select="current()"/>
            </xsl:if>
        </xsl:for-each>
    </xsl:template>

    <xsl:template name="link">
        <xsl:param name="req_type" select="'browse'"/>
        <xsl:param name="driver" select="'1'"/>
        <xsl:param name="object_id" select="'-1'"/>
        <xsl:param name="starting_index" select="'0'"/>
        <xsl:param name="requested_count" select="''"/>
        <xsl:param name="icon_path" select="'icons'"/>
        <xsl:param name="icon" select="'NULL'"/>
        <xsl:param name="roll_id" select="'NULL'"/>
        <xsl:param name="roll_value" select="'0'"/>
        <xsl:param name="text" select="'NULL'"/>
        <xsl:param name="sid" select="'NULL'"/>
        <xsl:param name="aux" select="''"/>

        <xsl:if test="$icon != 'NULL'">
        <xsl:choose>
            <xsl:when test="$roll_id = 'NULL'">
        
        <a href="interface?req_type={$req_type}&amp;object_id={$object_id}&amp;starting_index={$starting_index}&amp;driver={$driver}&amp;requested_count={$requested_count}&amp;sid={$sid}{$aux}"> 
            <img src="/{$icon_path}/{$icon}" border="0"/>
        </a>
            </xsl:when>
            <xsl:otherwise>
        <a href="interface?req_type={$req_type}&amp;object_id={$object_id}&amp;starting_index={$starting_index}&amp;driver={$driver}&amp;requested_count={$requested_count}&amp;sid={$sid}{$aux}" 
           onmouseover="activate_icon('{$roll_id}_{$roll_value}')" 
           onmouseout="deactivate_icon('{$roll_id}_{$roll_value}')">
            <img id="{$roll_id}_{$roll_value}" src="/{$icon_path}/{$icon}" border="0"/>
        </a>

            </xsl:otherwise>
        </xsl:choose>
        </xsl:if>
        <xsl:if test="$text != 'NULL'">
            <a href="interface?req_type={$req_type}&amp;object_id={$object_id}&amp;starting_index={$starting_index}&amp;driver={$driver}&amp;requested_count={$requested_count}&amp;sid={$sid}{$aux}">
                <xsl:value-of select="$text"/>
            </a>
        </xsl:if>
    </xsl:template>


    <xsl:template name="select_option">
        <xsl:param name="option_name" select="'NULL'"/>
        <xsl:param name="option_value" select="'NULL'"/>
        <xsl:param name="selected_value" select="'NULL'"/>
        <xsl:param name="last_index" select="'NULL'"/>

           <xsl:choose>
               <xsl:when test="$option_value = $selected_value">
                   <option value="{$option_value}&amp;starting_index={$last_index}" selected="true">
                        <xsl:value-of select="$option_name"/>
                   </option>
               </xsl:when>
               <xsl:otherwise>
                   <option value="{$option_value}&amp;starting_index={$last_index}">
                        <xsl:value-of select="$option_name"/>
                   </option>
               </xsl:otherwise>
           </xsl:choose>

    </xsl:template>

</xsl:transform>
