import js from "@eslint/js";
import promise from "eslint-plugin-promise";
import globals from "globals";

export default [
    {
        ignores: ["node_modules/**", "coverage/**", "_build/**", "dist/**"],
      },
    js.configs.recommended,
    {
        files: ["**/*.js", "**/*.cjs"],
        languageOptions: {
            ecmaVersion: 2022,
            sourceType: "module",
            globals: {
                  ...globals.browser,
                  ...globals.node,
                  ...globals.jquery,
                  ...globals.mocha,
                  ...globals.jasmine,
                  Cookies: "readonly",
                  },
            },
        plugins: {
            promise,
            },
        rules: {
            "no-unused-expressions": "off",
            "no-useless-escape": "warn",
            "no-unused-vars": [
                  "warn",
                  {
                          varsIgnorePattern: "event|should|expect|fixture|Cookies",
                        },
                  ],
            },
      },
];
