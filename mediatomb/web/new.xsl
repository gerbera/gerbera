<?xml version="1.0" encoding="UTF-8"?>
<xsl:transform version="1.0"
               xmlns:dc="http://purl.org/dc/elements/1.1/"
               xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
               xmlns:upnp="urn:schemas-upnp-org:metadata-1-0/upnp/">

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
                        <input name="req_type" value="new" type="hidden"/>
                        <input name="driver" value="{$current_driver}" type="hidden"/>
                        <input name="sid" value="{$sid}" type="hidden"/>
                        <input name="object_id" value="{$object_id}" type="hidden"/>
                        <input name="reload" value="0" type="hidden"/>
                        <table border="0" cellpadding="0" cellspacing="10">
                            <tbody>
                                <xsl:apply-templates select="/root/select"/>
                                <xsl:apply-templates select="/root/inputs"/>
                                <tr>
                                    <td/>
                                    <td>
                                        <div align="right">
                                            <input name="submit button" value="Add Object" type="submit"/>
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

    <xsl:template match="/root/select">
        <xsl:variable name="def">
            <xsl:value-of select="attribute::default"/>
        </xsl:variable>
     
        <tr>
            <td>Object Type:</td>
            <td>
                <select name="type" onChange="this.form.reload.value = 1; this.form.submit()">
                    <xsl:for-each select="option">
                        <option value="{attribute::type}">
                            <xsl:if test="$def = attribute::type">
                                <xsl:attribute name="selected">
                                    <xsl:value-of select="$def = attribute::type"/>
                                </xsl:attribute>
                            </xsl:if>
                            <xsl:value-of select="attribute::name"/>
                        </option>
                    </xsl:for-each>
                </select>
            </td>
        </tr>
    </xsl:template>

    <xsl:template match="/root/inputs">
        <xsl:for-each select="option">
            <tr>
                <td>
                    <xsl:value-of select="attribute::name"/>
                </td>
                <td>
                    <input type="text" size="24" name="{attribute::type}">
                        <xsl:if test="attribute::default != ''">
                            <xsl:attribute name="value">
                                <xsl:value-of select="attribute::default"/>
                            </xsl:attribute>
                        </xsl:if>
                    </input>
                </td>
            </tr>
        </xsl:for-each>
    </xsl:template>
</xsl:transform>    
