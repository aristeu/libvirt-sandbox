# This file is part of libvirt-sandbox.
#
# Copyright 2012 Dan Walsh
#
# systemd is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# systemd is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with systemd; If not, see <http://www.gnu.org/licenses/>.

__contains_word () {
    local word=$1; shift
    for w in $*; do [[ $w = $word ]] && return 0; done
    return 1
}

ALL_OPTS='-h --help'

__get_all_containers () {
    for i in `ls -1 /etc/libvirt-sandbox/services/*.sandbox 2>/dev/null`; do basename $i | cut -f1 -d.; done
}

__get_all_types () {
    seinfo -asvirt_lxc_domain -x 2>/dev/null | tail -n +2
}

_virt_sandbox_service () {
    local command=${COMP_WORDS[1]}
    local cur=${COMP_WORDS[COMP_CWORD]} prev=${COMP_WORDS[COMP_CWORD-1]}
    local verb comps
    local -A VERBS=(
           [CONNECT]='connect'
           [CREATE]='create'
           [DELETE]='delete'
           [RELOAD]='reload'
           [START]='start'
           [EXECUTE]='execute'
           [STOP]='stop'
           [LIST]='list'
    )
    local -A OPTS=(
        [ALL]='-h --help'
        [CREATE]='-e --executable -p --path -t --type -l --level -d --dynamic -n --clone -i --image -s --size'
        [LIST]='-r --running'
        [EXECUTE]='-C --command'
    )

    for ((i=0; $i <= $COMP_CWORD; i++)); do
        if __contains_word "${COMP_WORDS[i]}" ${VERBS[*]} &&
         ! __contains_word "${COMP_WORDS[i-1]}" ${OPTS[ARG}]}; then
            verb=${COMP_WORDS[i]}
            break
        fi
    done

    if test "$verb" = "" && test "$prev" = "virt-sandbox-service" ; then
        comps="${VERBS[*]}"
        COMPREPLY=( $(compgen -W "$comps" -- "$cur") )
        return 0
    elif test "$verb" == "list" ; then
        if test "$prev" = "-r" || test "$prev" = "--running" ; then
        return 0
        fi
        COMPREPLY=( $(compgen -W "${OPTS[ALL]} ${OPTS[LIST]} " -- "$cur") )
        return 0
    elif test "$verb" == "execute" ; then
        if test "$prev" = "-C" || test "$prev" = "--command" ; then
        COMPREPLY=( $( compgen -f -- "$cur") )
        compopt -o filenames
        return 0
        fi

        for ((i=0; $i <= $COMP_CWORD; i++)); do
        if __contains_word "${COMP_WORDS[i]}" ${OPTS[EXECUTE]}; then
            COMPREPLY=( $(compgen -W "$( __get_all_containers ) " -- "$cur") )
            return 0
        fi
        done
        COMPREPLY=( $(compgen -W "${OPTS[ALL]} ${OPTS[EXECUTE]} " -- "$cur") )
        return 0
    elif test "$verb" == "create" ; then
        if test "$prev" = "-e" || test "$prev" = "--executable" ; then
        COMPREPLY=( $( compgen -f -- "$cur") )
        compopt -o filenames
        return 0
        elif test "$prev" = "-p" || test "$prev" = "--path" ; then
        COMPREPLY=( $( compgen -d -- "$cur") )
        compopt -o filenames
        return 0
        elif test "$prev" = "-t" || test "$prev" = "--type" ; then
        COMPREPLY=( $(compgen -W "$( __get_all_types ) " -- "$cur") )
        return 0
        elif test "$prev" = "-l" || test "$prev" = "--level" ; then
        return 0
        elif test "$prev" = "-s" || test "$prev" = "--size" ; then
        return 0
        elif __contains_word "$command" ${VERBS[CREATE]} ; then
        COMPREPLY=( $(compgen -W "${OPTS[ALL]} ${OPTS[CREATE]}" -- "$cur") )
        return 0
        elif __contains_word "${COMP_WORDS[i]}" ${VERBS[*]} ; then
        COMPREPLY=( $(compgen -W "${OPTS[ALL]}" -- "$cur") )
        return 0
        fi
    else
        if ! __contains_word "${prev}" ${VERBS[*]} &&
            ! __contains_word "${prev}" ${OPTS[*]}; then
        return 0
        fi
    fi
    COMPREPLY=( $(compgen -W "${OPTS[ALL]} $( __get_all_containers ) " -- "$cur") )
    return 0
}
complete -F _virt_sandbox_service virt-sandbox-service
