#!/usr/bin/env bash
# Gerbera - https://gerbera.io/
#
# gerbera-completion.sh - this file is part of Gerbera.
#
# Copyright (C) 2025 Gerbera Contributors
#
# Gerbera is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2
# as published by the Free Software Foundation.
#
# Gerbera is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# NU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.
#
# $Id$

# Bash Completion for Gerbera
# To use this completion script, source it in your shell or place it in the appropriate completion directory.

_gerbera_completions() {
    local cur prev commands options
    COMPREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"
    
    commands="-D -c -d -h -v -u -P -f -p -i -e -m"
    commands="${commands} --help --debug --version --compile-info --create-config --create-example-config --create-advanced-config --check-config --print-options --offline --drop-tables --init-lastfm --daemon --user --pidfile --config --cfgdir --logfile --rotatelog --syslog --add-file --set-option --port --ip --interface --home --magic --import-mode --debug-mode"
    
    if [[ ${COMP_CWORD} -eq 1 ]]; then
        COMPREPLY=( $(compgen -W "${commands}" -- ${cur}) )
    else
        case "${prev}" in
            -D|-c|-d|-h|-v|-u|-P|-f|-p|-i|-e|-m|--user|--pidfile|--config|--cfgdir|--logfile|--rotatelog|--syslog|--add-file|--set-option|--port|--ip|--interface|--home|--magic|--import-mode|--debug-mode)
                # Provide file path completion for these commands
                COMPREPLY=( $(compgen -f -- ${cur}) )
                ;;
            *)
                COMPREPLY=( $(compgen -W "${commands}" -- ${cur}) )
                ;;
        esac
    fi
    return 0
}

complete -F _gerbera_completions gerbera
