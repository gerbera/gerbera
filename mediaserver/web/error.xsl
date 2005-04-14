<?xml version="1.0" encoding="ISO-8859-1"?>
<xsl:transform version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

    <xsl:output method="html"/>

    <xsl:variable name="bgcolor_1">408bff</xsl:variable> <!-- darker blue -->
    <xsl:variable name="bgcolor_2">40a3ff</xsl:variable> <!-- lighter blue -->
    <xsl:variable name="link_color">black</xsl:variable>
    <xsl:variable name="text_color">white</xsl:variable>
    <xsl:variable name="icon_cell_width">31</xsl:variable>
    <xsl:variable name="icon_cell_height">31</xsl:variable> 


    <xsl:template match="/">
        <html>
    	    <head>
    		    <meta http-equiv="content-type" content="text/html;charset=iso-8859-1"/>
    		    <title>mediatomb</title>
    	    </head>

    	    <body bgcolor="#{$bgcolor_1}" text="{$link_color}">
    		    <div align="center">
                    <br/><br/>
                    <h3>
                        <xsl:value-of select="/root/display"/>
                    <br/><br/>
                        Error message:
                        <font color="{$text_color}">
                            <xsl:value-of select="/root/error"/>
                        </font>
                    </h3>                        
                </div>
    	    </body>
        </html>
    </xsl:template>
</xsl:transform>    
