<?xml version="1.0" encoding="ISO-8859-1"?>

<xsl:transform version="1.0"
    xmlns:dc="http://purl.org/dc/elements/1.1/"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:upnp="urn:schemas-upnp-org:metadata-1-0/upnp/">

<xsl:output method="html" indent="no"/>
    
<xsl:template match="/">
    <html>
        <head>
            <link rel="stylesheet" type="text/css" href="/main.css" title="style"/>
        </head>
        <body>
            <h3>Currently browsing: <xsl:value-of select="/root/current_browse/container/dc:title"/></h3>
            <table border="1">
                <tr bgcolor="#a0a0a0">
                    <th align="left">Object ID</th>
                    <th align="left">Title</th>
                    <th align="left">Class</th>
<!--                    <th align="left">Resource</th> -->
                </tr>
                <tr>
                    <td/>
                    <xsl:choose>
                        <xsl:when test="/root/current_browse/container/attribute::parentID &lt; 0">
                            <td/>
                        </xsl:when>
                        <xsl:otherwise>
                            <xsl:variable name="parent">
                                <xsl:value-of select="/root/current_browse/container/attribute::parentID"/>
                            </xsl:variable>
                            <td>
                                <a href="interface?object_id={$parent}&amp;req_type=browse">Up</a>
                            </td>
                        </xsl:otherwise>
                    </xsl:choose>
                    <td/>
                </tr>
                <xsl:for-each select="/root/DIDL-Lite/container">
                    <tr>
                        <td><xsl:value-of select="attribute::id"/></td>
                        <xsl:choose>
                            <xsl:when test="attribute::childCount &gt; 0">
                                <xsl:variable name="object_id">
                                    <xsl:value-of select="attribute::id"/>
                                </xsl:variable>
                                <td>
                                    <a href="interface?object_id={$object_id}&amp;req_type=browse">
                                        <xsl:value-of select="dc:title"/>
                                    </a>
                                </td>
                            </xsl:when>
                            <xsl:otherwise>
                                <td>
                                    <xsl:value-of select="dc:title"/>
                                </td>
                            </xsl:otherwise>
                        </xsl:choose>
                        <td><xsl:value-of select="upnp:class"/></td>
<!--                        <td><xsl:value-of select="res"/></td> -->
                    </tr>
               </xsl:for-each>
               <xsl:for-each select="/root/DIDL-Lite/item">
               <xsl:sort select="dc:title"/> 
                    <tr>
                        <td><xsl:value-of select="attribute::id"/></td>

                        <xsl:variable name="link">
                            <xsl:value-of select="res"/>
                        </xsl:variable>
                        
                        <td>
                            <a href="{$link}" target="blank">
                                <xsl:value-of select="dc:title"/>
                            </a>
                        </td>
                        <td><xsl:value-of select="upnp:class"/></td>

                    </tr>
                </xsl:for-each>
            </table>
        </body>
    </html>
</xsl:template>

</xsl:transform>
