<?xml version="1.0" encoding="UTF-8"?>
<xsl:transform version="1.0"
               xmlns:dc="http://purl.org/dc/elements/1.1/"
               xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
               xmlns:upnp="urn:schemas-upnp-org:metadata-1-0/upnp/">

    <xsl:output method="html"/>

    <xsl:include href="/defaults.xsl"/>

    
    <xsl:variable name="current_driver">
      <xsl:value-of select="/root/driver"/>
    </xsl:variable>

    <xsl:variable name="sid">
        <xsl:value-of select="/root/sid"/>
    </xsl:variable>
    
    <xsl:variable name="object_id">
        <xsl:value-of select="/root/object_id"/>
    </xsl:variable>
    <xsl:template match="/">
        <html>
    	    <head>
    		    <meta http-equiv="content-type" content="text/html;charset=iso-8859-1"/>
    		    <title>mediaserver</title>
             <script language="JavaScript" type="text/javascript" src="/rollover.js">
             </script>
 
    	    </head>

    	    <body bgcolor="#{$bgcolor_1}" link="black" text="white">
    		    <div align="center">

                    ||
                
                    <xsl:call-template name="link"> 
                        <xsl:with-param name="text" select="'Browse Database'"/>
                        <xsl:with-param name="sid" select="$sid"/>
                    </xsl:call-template>

                    || 
                    
                    <xsl:call-template name="link"> 
                        <xsl:with-param name="driver"  select="'2'"/>
                        <xsl:with-param name="text" select="'Browse Filesystem'"/>
                        <xsl:with-param name="sid" select="$sid"/>
                    </xsl:call-template>

                    ||

                    <a href="/help.html">Help</a>

                    ||
                    
                    <br/>
                    <br/>
                    <form action="/content/interface" method="get" name="Edit">
                        <input name="req_type" value="edit_save" type="hidden"/>
                        <input name="driver" value="{$current_driver}" type="hidden"/>
                        <input name="sid" value="{$sid}" type="hidden"/>
                        <input name="object_id" value="{$object_id}" type="hidden"/>
                        <table border="0" cellpadding="0" cellspacing="10">
                            <tbody>
                                <xsl:apply-templates select="/root/container"/>
                                <xsl:apply-templates select="/root/item"/>
                                <tr>
                                    <td/>
                                    <td>
                                        <div align="right">
                                            <input name="submit" value="Submit Changes" type="submit"/>
                                        </div>
                                    </td>
                                </tr>
                            </tbody>
                        </table>
                    </form>
                </div>
    	    </body>
        </html>
    </xsl:template>

    <xsl:template match="/root/container">
        <tr>
            <td>Title:</td>
            <td><input name="title" size="24" value="{dc:title}" type="text"/></td>
         </tr>
        <tr>
            <td>UPnP Class:</td>
            <td><input name="class" size="24" value="{upnp:class}" type="text"/></td>
        </tr>
<!--         
        <tr>
            <td>Autoscan:</td>
            <td><input value="autoscan" name="autoscan" type="checkbox"/></td>
        </tr>
-->
        <tr/>
    </xsl:template>

    <xsl:template match="/root/item">
        <tr>
            <td>Title:</td>
            <td><input name="title" size="24" value="{dc:title}" type="text"/></td>
        </tr>
        <xsl:choose>
            <xsl:when test="object_type = 'url'">
                <tr>
                    <td>URL</td>
                    <td><input name="location" size="24" value="{location}" type="text"/></td>
                </tr>
                <tr>
                    <td>Protocol</td>
                    <td><input name="protocol" size="24" value="{substring-before(res/attribute::protocolInfo, ':')}" type="text"/></td>
                </tr>
            </xsl:when>
            <xsl:otherwise>
                <tr>
                    <td>Location</td>
                    <td><input name="location" size="24" value="{location}" type="text"/></td>
                </tr>
            </xsl:otherwise>
        </xsl:choose>
        <tr>
            <td>UPnP Class:</td>
            <td><input name="class" size="24" value="{upnp:class}" type="text"/></td>
        </tr>
        <tr>
            <td>Description:</td>
            <td><input name="description" size="24" value="{dc:description}" type="text"/></td>
        </tr>

        <tr>
            <td>Mime Type:</td>
            <td><input name="mime-type" size="24" value="{substring-before(substring-after(substring-after(res/attribute::protocolInfo, ':'), ':'), ':')}" type="text"/></td>
        </tr>
        <xsl:if test="object_type = 'act'">
            <tr>
                <td>Action:</td>
                <td><input name="action" size="24" value="{action}" type="text"/></td>
            </tr>
            <tr>
                <td>State:</td>
                <td><input name="state" size="24" value="{state}" type="text"/></td>
            </tr>

        </xsl:if>
        <tr/>
    </xsl:template>

</xsl:transform>    
