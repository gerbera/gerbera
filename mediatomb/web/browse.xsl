<?xml version="1.0" encoding="UTF-8"?>
<xsl:transform version="1.0"
               xmlns:dc="http://purl.org/dc/elements/1.1/"
               xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
               xmlns:upnp="urn:schemas-upnp-org:metadata-1-0/upnp/">

<!-- browse.xsl  - this file is part of MediaTomb.
                                                                                
    Copyright (C) 2005 Gena Batyan <bgeradz@deadlock.dhs.org>,
                       Sergey Bostandzhyan <jin@deadlock.dhs.org>
                                                                                
    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
                                                                                
    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
                                                                                
    You should have received a copy of the GNU General Public License
    along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
-->

    <xsl:output method="html"/>

    <xsl:include href="/defaults.xsl"/>

    <xsl:variable name="parent_container">
        <xsl:choose>
            <xsl:when test="/root/current_browse/container/attribute::parentID != -1">
                1 
            </xsl:when>
            <xsl:otherwise>
                0
            </xsl:otherwise>
        </xsl:choose>            
    </xsl:variable>

    <xsl:variable name="containers">
        <xsl:value-of select="count(/root/DIDL-Lite/container)"/>
    </xsl:variable>

    <xsl:variable name="current_container">
        <xsl:value-of select="/root/current_browse/container/attribute::id"/>
    </xsl:variable>

    <xsl:variable name="items_per_page">
      <xsl:value-of select="/root/current_browse/page/LastRequestedCount"/>
    </xsl:variable>
    
     <xsl:variable name="current_driver">
      <xsl:value-of select="/root/current_browse/driver"/>
    </xsl:variable>

    <xsl:variable name="sid">
        <xsl:value-of select="/root/current_browse/sid"/>
    </xsl:variable>

    <xsl:variable name="options_cell_width">
        <xsl:variable name="current">
            <xsl:value-of select="count(/root/current_browse/actions/current)"/>
        </xsl:variable>

        <xsl:variable name="common">
            <xsl:value-of select="count(/root/current_browse/actions/common)"/>
        </xsl:variable>
        
        <xsl:variable name="container">
            <xsl:value-of select="count(/root/current_browse/actions/container)"/>
        </xsl:variable>  
        <xsl:variable name="item">
            <xsl:value-of select="count(/root/current_browse/actions/item)"/>
        </xsl:variable>

        <xsl:variable name="containers">
            <xsl:value-of select="$common + $container"/>
        </xsl:variable>
        
        <xsl:variable name="items">
            <xsl:value-of select="$common + $item"/>
        </xsl:variable>
        
        <xsl:variable name="max">
            <xsl:choose>
                <xsl:when test="$containers &gt; $items">
                    <xsl:value-of select="$containers"/>
                </xsl:when>
                <xsl:otherwise>
                    <xsl:value-of select="$items"/>
                </xsl:otherwise>
            </xsl:choose>
        </xsl:variable>

        <xsl:choose>
            <xsl:when test="$current &gt; $max">
                <xsl:value-of select="$current * $icon_cell_width"/>
            </xsl:when>
            <xsl:otherwise>
                <xsl:value-of select="$max * $icon_cell_width"/>
            </xsl:otherwise>
        </xsl:choose> 
    </xsl:variable> 
    
    <xsl:template match="/">
        <html>
    	    <head>
    		    <meta http-equiv="content-type" content="text/html;charset=iso-8859-1"/>
    		    <title>MediaTomb</title>
             <script language="JavaScript" type="text/javascript" src="/rollover.js">
             </script>
 
    	    </head>

    	    <body bgcolor="#{$bgcolor_1}" link="black" text="white">
            
    		    <div align="center">

                    ||
               
                    <xsl:choose>
                        <xsl:when test="$current_driver = '1'">
                            Browse Database
                            
                            || 
                    
                            <xsl:call-template name="link"> 
                                <xsl:with-param name="driver"  select="'2'"/>
                                <xsl:with-param name="text" select="'Browse Filesystem'"/>
                                <xsl:with-param name="sid" select="$sid"/>
                            </xsl:call-template>
                        </xsl:when>
                        <xsl:otherwise>

                            <xsl:call-template name="link"> 
                                <xsl:with-param name="text" select="'Browse Database'"/>
                                <xsl:with-param name="sid" select="$sid"/>
                            </xsl:call-template>

                            || 
                    
                            Browse Filesystem
                        </xsl:otherwise>
                    </xsl:choose>

                    ||

                    <a href="/help.html">Help</a> 
                   
                    ||
                    
                    <br/>
                    <br/>

                    <xsl:apply-templates select="/root/current_browse/page">
                        <xsl:with-param name="IMAGE_VAL" select="'_0'"/>
                        <xsl:with-param name="driver" select="$current_driver"/>
                        <xsl:with-param name="sid" select="$sid"/>
                    </xsl:apply-templates>
<!--
                    <xsl:call-template name="renderCurrent">
                        <xsl:with-param name="driver" select="$current_driver"/>
                        <xsl:with-param name="sid" select="$sid"/>
                        <xsl:with-param name="current_object_id" select="$current_container"/>
                    </xsl:call-template>
-->
    			    <table border="1" cellpadding="0" cellspacing="0" width="100%">
                        <xsl:apply-templates select="/root/current_browse/path">
                            <xsl:with-param name="driver" select="$current_driver"/>
                            <xsl:with-param name="sid" select="$sid"/>
                            <xsl:with-param name="current_object_id" select="$current_container"/>
                        </xsl:apply-templates>
                        <xsl:apply-templates select="/root/DIDL-Lite">
                            <xsl:with-param name="driver" select="$current_driver"/>
                            <xsl:with-param name="sid" select="$sid"/>
                        </xsl:apply-templates>                            
                    </table>

                    <br/>

                    <xsl:apply-templates select="/root/current_browse/page">
                        <xsl:with-param name="IMAGE_VAL" select="'_1'"/>
                        <xsl:with-param name="driver" select="$current_driver"/>
                        <xsl:with-param name="sid" select="$sid"/>
                    </xsl:apply-templates>

    		        <form name="Add new content" action="/content/interface" method="get">
                        <input name="req_type" value="add" type="hidden"/>
                        <input name="driver" value="1" type="hidden"/>
                        <input name="sid" value="{$sid}" type="hidden"/>
    			        <input type="input" name="path" size="20"/>
                        <input type="submit" name="submitButtonName" value="Add"/>
    		        </form>
                </div>
    	    </body>
        </html>
    </xsl:template>

    <xsl:template match="/root/current_browse/page">
        <xsl:param name="IMAGE_VAL" select="'default'"/>
        <xsl:param name="driver" select="'1'"/> 
        <xsl:param name="sid" select="''"/> 

        <xsl:variable name="current_index">
            <xsl:value-of select="CurrentIndex"/>
        </xsl:variable>
        
        <xsl:variable name="last_index">
            <xsl:value-of select="LastStartingIndex"/>
        </xsl:variable>

        <xsl:variable name="total_matches">
            <xsl:value-of select="TotalMatches"/>
        </xsl:variable>

        <form method="GET">
        <table border="0" cellpadding="0" cellspacing="0" height="{$icon_cell_height}">
            <tr>
                <td width="{$icon_cell_width}" height="{$icon_cell_height}" align="center" valign="middle">
                    <xsl:if test="(CurrentIndex &gt; $items_per_page) and ($items_per_page &gt; 0)">
                        <xsl:call-template name="link"> <!-- browse to first page -->
                            <xsl:with-param name="object_id"        select="$current_container"/>
                            <xsl:with-param name="driver"           select="$driver"/>
                            <xsl:with-param name="starting_index"   select="0"/>
                            <xsl:with-param name="requested_count"  select="$items_per_page"/>
                            <xsl:with-param name="icon"             select="$icon_top_list"/>
                            <xsl:with-param name="roll_id"          select="'db_go_to_top'"/>
                            <xsl:with-param name="roll_value"       select="$IMAGE_VAL"/>
                            <xsl:with-param name="sid" select="$sid"/>
                        </xsl:call-template>
                   </xsl:if>
                </td>
                <td width="{$icon_cell_width}" height="{$icon_cell_height}" align="center" valign="middle">
                    <xsl:if test="(CurrentIndex &gt; $items_per_page) and ($items_per_page &gt; 0)">
                        <xsl:call-template name="link"> <!-- go one page back -->
                            <xsl:with-param name="object_id"        select="$current_container"/>
                            <xsl:with-param name="driver"           select="$driver"/>
                            <xsl:with-param name="starting_index"   select="$last_index - $items_per_page"/>
                            <xsl:with-param name="requested_count"  select="$items_per_page"/>
                            <xsl:with-param name="icon"             select="$icon_page_up"/>
                            <xsl:with-param name="roll_id"          select="'db_page_up'"/>
                            <xsl:with-param name="roll_value"       select="$IMAGE_VAL"/>
                            <xsl:with-param name="sid" select="$sid"/>
                       </xsl:call-template>
                   </xsl:if>
                </td>

                <td width="{$icon_cell_width}" height="{$icon_cell_height}" align="center" valign="middle">
                        <input name="req_type" value="browse" type="hidden"/>
                        <input name="object_id" value="{$current_container}" type="hidden"/>
                        <input name="driver" value="{$driver}" type="hidden"/>
                        <input name="sid" value="{$sid}" type="hidden"/>
                        
                        <select name="requested_count" onChange="this.form.submit()">
                        <xsl:call-template name="select_option">
                            <xsl:with-param name="option_name"  select="'15'"/>
                            <xsl:with-param name="option_value" select="'15'"/>
                            <xsl:with-param name="selected_value" select="$items_per_page"/>
                            <xsl:with-param name="last_index" select="$last_index"/>
                        </xsl:call-template>
                        <xsl:call-template name="select_option">
                            <xsl:with-param name="option_name"  select="'30'"/>
                            <xsl:with-param name="option_value" select="'30'"/>
                            <xsl:with-param name="selected_value" select="$items_per_page"/>
                            <xsl:with-param name="last_index" select="$last_index"/>
                        </xsl:call-template>
                        <xsl:call-template name="select_option">
                            <xsl:with-param name="option_name"  select="'60'"/>
                            <xsl:with-param name="option_value" select="'60'"/>
                            <xsl:with-param name="selected_value" select="$items_per_page"/>
                            <xsl:with-param name="last_index" select="$last_index"/>
                        </xsl:call-template>
                        <xsl:call-template name="select_option">
                            <xsl:with-param name="option_name"  select="'100'"/>
                            <xsl:with-param name="option_value" select="'100'"/>
                            <xsl:with-param name="selected_value" select="$items_per_page"/>
                            <xsl:with-param name="last_index" select="$last_index"/>
                        </xsl:call-template>
                        <xsl:call-template name="select_option">
                            <xsl:with-param name="option_name"  select="'All'"/>
                            <xsl:with-param name="option_value" select="'0'"/>
                            <xsl:with-param name="selected_value" select="$items_per_page"/>
                            <xsl:with-param name="last_index" select="0"/>
                        </xsl:call-template>


                        </select>
                </td>
                <td width="{$icon_cell_width}" height="{$icon_cell_height}" align="center" valign="middle">
                    <xsl:if test="(CurrentIndex &lt; TotalMatches) and ($items_per_page &gt; 0)">
                        <xsl:call-template name="link"> <!-- go to the next page in the list -->
                            <xsl:with-param name="object_id"        select="$current_container"/>
                            <xsl:with-param name="driver"           select="$driver"/>
                            <xsl:with-param name="starting_index"   select="$current_index"/>
                            <xsl:with-param name="requested_count"  select="$items_per_page"/>
                            <xsl:with-param name="icon"             select="$icon_page_down"/>
                            <xsl:with-param name="roll_id"          select="'db_page_down'"/>
                            <xsl:with-param name="roll_value"       select="$IMAGE_VAL"/>
                            <xsl:with-param name="sid" select="$sid"/>
                       </xsl:call-template>
                    </xsl:if>
                </td>
                <td width="{$icon_cell_width}" height="{$icon_cell_height}" align="center" valign="middle">
                    <xsl:if test="(CurrentIndex &lt; TotalMatches) and ($items_per_page &gt; 0)">
                        <xsl:call-template name="link"> <!-- go to the last page in the list -->
                            <xsl:with-param name="object_id"        select="$current_container"/>
                            <xsl:with-param name="driver"           select="$driver"/>
                            <xsl:with-param name="starting_index"   select="$total_matches - $items_per_page"/>
                            <xsl:with-param name="requested_count"  select="$items_per_page"/>
                            <xsl:with-param name="icon"             select="$icon_bottom_list"/>
                            <xsl:with-param name="roll_id"          select="'db_go_to_bottom'"/>
                            <xsl:with-param name="roll_value"       select="$IMAGE_VAL"/>
                            <xsl:with-param name="sid" select="$sid"/>
                       </xsl:call-template>
                    </xsl:if>
                </td>
            </tr>
        </table>
        </form>
    </xsl:template>

    <xsl:template name="renderCaption" match="/root/current_browse/path"> <!-- render the path in caption for quick navigation between containers  -->
    <xsl:param name="driver" select="'1'"/>
    <xsl:param name="sid" select="''"/>
    <xsl:param name="current_object_id" select="''"/>
        <tr>
            <td>
                <div style="border: solid 0 white; border-bottom-width:1px;">
                    <table border="0"  cellpadding="1" cellspacing="0" width="100%">
                        <tr>
                            <td width="1">
                                <img src="/{$icons_path}/{$icon_current}" border="0"/>
                            </td>
                            <td align="center">
                                <b>
                                    <xsl:choose>
                                        <xsl:when test="$driver = 1">
                                            &#187;
                                        </xsl:when>
                                        <xsl:otherwise>
                                            <xsl:choose>
                                                <xsl:when test="$current_object_id = 0">
                                                    Filesystem 
                                                </xsl:when>
                                                <xsl:otherwise>
                                                    <xsl:call-template name="link">
                                                        <xsl:with-param name="object_id"        select="'0'"/>
                                                        <xsl:with-param name="driver"           select="$driver"/>
                                                        <xsl:with-param name="requested_count"  select="$items_per_page"/>
                                                        <xsl:with-param name="text"             select="'Filesystem'"/>
                                                        <xsl:with-param name="sid"             select="$sid"/>
                                                    </xsl:call-template>
                                                </xsl:otherwise>
                                            </xsl:choose>
                                        </xsl:otherwise>
                                    </xsl:choose>
                                    <xsl:for-each select="container">
                                        <xsl:choose>
                                            <xsl:when test="$current_container = attribute::id">
                                                <xsl:value-of select="dc:title"/>
                                            </xsl:when>
                                            <xsl:otherwise>
                                                <xsl:call-template name="link">
                                                    <xsl:with-param name="object_id"        select="attribute::id"/>
                                                    <xsl:with-param name="driver"           select="$driver"/>
                                                    <xsl:with-param name="requested_count"  select="$items_per_page"/>
                                                    <xsl:with-param name="text"             select="dc:title"/>
                                                    <xsl:with-param name="sid"             select="$sid"/>
                                                </xsl:call-template>
                                                <xsl:choose>
                                                    <xsl:when test="$driver = 1">
                                                        &#187;
                                                    </xsl:when>
                                                    <xsl:otherwise>
                                                        / 
                                                    </xsl:otherwise>
                                                </xsl:choose>
                                            </xsl:otherwise>
                                        </xsl:choose>
                                    </xsl:for-each>
                                </b>
                            </td>
                        </tr>
                    </table>
                </div>
            </td>
            <td width="{$options_cell_width}">
                <div style="border: solid 0 white; border-bottom-width:1px;">
                    <table border="0" cellpadding="0" cellspacing="0">
                        <tr> 
                            <xsl:apply-templates select="/root/current_browse/actions">
                                <xsl:with-param name="object_id" select="$current_object_id"/>
                                <xsl:with-param name="driver" select="$driver"/>
                                <xsl:with-param name="type" select="'current'"/>
                                <xsl:with-param name="sid" select="$sid"/>
                                <xsl:with-param name="pos" select="position()"/>
                            </xsl:apply-templates>
                        </tr>
                    </table>
                </div>
            </td>
        </tr>
    </xsl:template> <!-- renderCaption -->

    <xsl:template name="addActions" match="/root/current_browse/actions"> <!-- add action links/icons to items or containers -->
        <xsl:param name="type" select="'container'"/>
        <xsl:param name="object_id" select="''"/>
        <xsl:param name="driver" select="'1'"/>
        <xsl:param name="sid" select="''"/>
        <xsl:param name="pos" select="position()"/>

        <xsl:choose>
            <xsl:when test="$type = 'current'">
                <xsl:for-each select="current">
                    <td width="{$icon_cell_width}" height="{$icon_cell_height}" align="center" valign="middle">
                        <xsl:variable name="current_icon">
                            <xsl:call-template name="get_icon">
                                <xsl:with-param name="name" select="attribute::req_type"/>
                            </xsl:call-template>
                        </xsl:variable>

                        <xsl:call-template name="link"> 
                            <xsl:with-param name="req_type"         select="attribute::req_type"/> 
                            <xsl:with-param name="driver"           select="$driver"/> 
                            <xsl:with-param name="object_id"        select="$object_id"/>
                            <xsl:with-param name="requested_count"  select="$items_per_page"/>
                            <xsl:with-param name="icon"             select="$current_icon"/>
                            <xsl:with-param name="roll_id"          select="concat(concat('action', $type), attribute::req_type)"/>
                            <xsl:with-param name="roll_value"       select="$pos"/>
                            <xsl:with-param name="sid"             select="$sid"/>
                        </xsl:call-template>
                    </td>
                </xsl:for-each>
            </xsl:when>
            <xsl:otherwise>
                <xsl:for-each select="common">
                    <td width="{$icon_cell_width}" height="{$icon_cell_height}" align="center" valign="middle">
                        <xsl:variable name="current_icon">
                            <xsl:call-template name="get_icon">
                                <xsl:with-param name="name" select="attribute::req_type"/>
                            </xsl:call-template>
                        </xsl:variable>

                        <xsl:call-template name="link"> 
                            <xsl:with-param name="req_type"         select="attribute::req_type"/> 
                            <xsl:with-param name="driver"           select="$driver"/> 
                            <xsl:with-param name="object_id"        select="$object_id"/>
                            <xsl:with-param name="requested_count"  select="$items_per_page"/>
                            <xsl:with-param name="icon"             select="$current_icon"/>
                            <xsl:with-param name="roll_id"          select="concat(concat('action', $type), attribute::req_type)"/>
                            <xsl:with-param name="roll_value"       select="$pos"/>
                            <xsl:with-param name="sid"             select="$sid"/>
                        </xsl:call-template>
                    </td>
                </xsl:for-each>
            </xsl:otherwise>
        </xsl:choose>
    </xsl:template> <!-- addActions --> 
   
    <xsl:template name="renderCurrent">
        <xsl:param name="driver" select="'1'"/>
        <xsl:param name="sid" select="''"/>
        <xsl:param name="current_object_id" select="''"/>

        <xsl:variable name="title">
            <xsl:value-of select="/root/current_browse/container/dc:title"/>
        </xsl:variable>
      
                <xsl:attribute name="style"> 
                    background-color:#<xsl:value-of select="$bgcolor_1"/>;
                </xsl:attribute>
                    <b><xsl:value-of select="$title"/></b>
                    <table border="0" cellpadding="0" cellspacing="0">
                        <tr>
                            <xsl:apply-templates select="/root/current_browse/actions">
                                <xsl:with-param name="object_id" select="$current_object_id"/>
                                <xsl:with-param name="driver" select="$driver"/>
                                <xsl:with-param name="type" select="'current'"/>
                                <xsl:with-param name="sid" select="$sid"/>
                                <xsl:with-param name="pos" select="position()"/>
                            </xsl:apply-templates>
                        </tr>
                    </table>
    </xsl:template> <!-- renderCurrent -->

    <xsl:template name="renderDIDL" match="DIDL-Lite"> <!-- this template will render all containers and items -->
        <xsl:param name="driver" select="'1'"/>
        <xsl:param name="sid" select="''"/>
        <xsl:variable name="parent">
            <xsl:value-of select="/root/current_browse/container/attribute::parentID"/>
        </xsl:variable>

<!--        <xsl:if test="$parent != -1"> 
            <tr>
            <xsl:attribute name="style"> 
            <xsl:choose>
                <xsl:when test="($parent_container mod 2) = 0">
                    background-color:#<xsl:value-of select="$bgcolor_1"/>;
                </xsl:when>
                <xsl:otherwise>
                    background-color:#<xsl:value-of select="$bgcolor_2"/>;
                </xsl:otherwise>
            </xsl:choose>
            </xsl:attribute>
                <td valign="middle">
                    <xsl:call-template name="link"> 
                        <xsl:with-param name="object_id"        select="$parent"/>
                        <xsl:with-param name="driver"           select="$driver"/>
                        <xsl:with-param name="requested_count"  select="$items_per_page"/>
                        <xsl:with-param name="icon"             select="$icon_directory_up"/>
                        <xsl:with-param name="text"             select="'..'"/>
                        <xsl:with-param name="sid"              select="$sid"/>
                    </xsl:call-template>
                </td>
                <td/>
            </tr>
        </xsl:if> -->
        <xsl:for-each select="container"> <!-- first we go through the containrs -->

            <tr>
            <xsl:attribute name="style"> 
            <xsl:choose>
                <xsl:when test="((position()) mod 2) = 0"> <!-- alternate between the colors, so each table row has its own color -->
                    background-color:#<xsl:value-of select="$bgcolor_1"/>;
                </xsl:when>
                <xsl:otherwise>
                    background-color:#<xsl:value-of select="$bgcolor_2"/>;
                </xsl:otherwise>
            </xsl:choose>
            </xsl:attribute>
            
            <xsl:variable name="object_id">
                <xsl:value-of select="attribute::id"/>
            </xsl:variable>

    	        <td valign="middle">
                    <xsl:call-template name="link"> <!-- container link: browse a container -->
                        <xsl:with-param name="object_id"        select="$object_id"/> 
                        <xsl:with-param name="driver"           select="$driver"/>
                        <xsl:with-param name="requested_count"  select="$items_per_page"/>
                        <xsl:with-param name="icon"             select="$icon_container"/>
                        <xsl:with-param name="text"             select="dc:title"/>
                        <xsl:with-param name="sid"              select="$sid"/>
                    </xsl:call-template>
                </td>
                <td>
                <xsl:if test="$object_id != 0">
                    <table border="0" cellpadding="0" cellspacing="0">
                        <tr>
                            <xsl:apply-templates select="/root/current_browse/actions">
                                <xsl:with-param name="object_id" select="$object_id"/>
                                <xsl:with-param name="driver" select="$driver"/>
                                <xsl:with-param name="type" select="'container'"/>
                                <xsl:with-param name="sid" select="$sid"/>
                                <xsl:with-param name="pos" select="position()"/>
                            </xsl:apply-templates>
                        </tr>
                    </table>
                </xsl:if>
                </td>
            </tr>
        </xsl:for-each>
        <xsl:for-each select="item"> <!-- finally go through all items -->
        
            <tr>
            <xsl:attribute name="style"> 
            <xsl:choose>
                <xsl:when test="(($containers + position()) mod 2) = 0"> <!-- also for items: alternate color of the table rows -->
                    background-color:#<xsl:value-of select="$bgcolor_1"/>;
                </xsl:when>
                <xsl:otherwise>
                    background-color:#<xsl:value-of select="$bgcolor_2"/>;
                </xsl:otherwise>
            </xsl:choose>
            </xsl:attribute>

            <xsl:variable name="object_id">
                <xsl:value-of select="attribute::id"/>
            </xsl:variable>

                <td valign="middle">
                    <xsl:variable name="link">
                        <xsl:value-of select="res"/>
                    </xsl:variable>
                    <a href="{$link}" target="blank">
                        <img src="/{$icons_path}/{$icon_item}" border="0"/>
                    </a>
                    <a href="{$link}" target="blank">
                        <xsl:value-of select="dc:title"/>
                    </a>
                </td>
                <td>
                <xsl:if test="$object_id != 0">
                    <table border="0" cellpadding="0" cellspacing="0">
                        <tr>
                            <xsl:apply-templates select="/root/current_browse/actions">
                                <xsl:with-param name="object_id" select="$object_id"/>
                                <xsl:with-param name="driver" select="$driver"/>
                                <xsl:with-param name="type" select="'item'"/>
                                <xsl:with-param name="sid" select="$sid"/>
                                <xsl:with-param name="pos" select="position()"/>
                            </xsl:apply-templates>
                        </tr>
                    </table>
                </xsl:if>
                </td>
            </tr>
        </xsl:for-each>
    </xsl:template> <!-- renderDIDL -->
</xsl:transform>    
