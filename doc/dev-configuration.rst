.. index:: Configuration Options

Implement Configuration Options
===============================

+------------------------------------+--------------------------------------------------------------------------+
| File                               | Action                                                                   |
+====================================+==========================================================================+
| ``src/config/config_val.h``        | Add enum member to ``ConfigVal`` before ``MAX``, values after            |
|                                    | ``MAX`` are for attributes and auxiliary options.                        |
+------------------------------------+--------------------------------------------------------------------------+
| ``src/config/config_definition.cc``| Define logic for loading in vector ``complexOptions``.                   |
|                                    |                                                                          |
|                                    | There are several ConfigSetup sub types that support automatic           |
|                                    | parsing,                                                                 |
|                                    |                                                                          |
|                                    | e.g. Numbers, Strings, Booleams, Paths, Arrays and Dictionaries.         |
+                                    +--------------------------------------------------------------------------+
|                                    | Add support for sub attributes in web ui in map ``parentOptions``.       |
+                                    +--------------------------------------------------------------------------+
|                                    | Avoid automatic loading by adding to map ``dependencyMap``.              |
+------------------------------------+--------------------------------------------------------------------------+
| ``src/config/config_manager.cc``   | Add special treatment and verifications if necessary.                    |
+------------------------------------+--------------------------------------------------------------------------+
| ``src/config/config_generator.cc`` | Add here if you want it to be part of the generated default config or    |
|                                    | example config file.                                                     |
|                                    |                                                                          |
|                                    | In this case do not forget to updated the fixtures                       |
|                                    | under ``test/config/fixtures``.                                          |
+------------------------------------+--------------------------------------------------------------------------+
| ``web/gerbera-config-*.json``      | Add entries for display and editing in web ui.                           |
+------------------------------------+--------------------------------------------------------------------------+
| ``doc/config*.rst``                | Supply some useful documentation.                                        |
+------------------------------------+--------------------------------------------------------------------------+
| ``config/config2.xsd``             | Adjust xsd for config file validation.                                   |
+------------------------------------+--------------------------------------------------------------------------+
