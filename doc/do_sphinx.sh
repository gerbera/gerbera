#!/usr/bin/env bash
ROOT_PATH=$PWD
DOC_BUILD_DIR=$PWD/_build
DOC_DIST_DIR=$DOC_BUILD_DIR/dist
VENV_NAME=gerbera-env
SPHINX_THEME=sphinx_rtd_theme
SPHINX_BUILDER=html

function create {
    if [ -d "$DOC_BUILD_DIR" ]
    then
      printf "\nRemoving $DOC_BUILD_DIR \n\n"
      rm -Rf $DOC_BUILD_DIR
    fi

    printf "\nCreating Gerbera documentation build directory $DOC_BUILD_DIR\n\n"
    mkdir $DOC_BUILD_DIR
    mkdir $DOC_DIST_DIR
    cd $DOC_BUILD_DIR
}

function setup {
    hash virtualenv 2>/dev/null || { echo >&2 "Script requires Python virtualenv but it's not installed.  Aborting."; exit 1; }
    printf "\nCreating Gerbera Python virtualenv\n\n"
    python3 -m virtualenv $VENV_NAME
    source $VENV_NAME/bin/activate
}

function install {
    printf "\nInstalling Sphinx document generator with $SPHINX_THEME theme\n\n"
    pip install Sphinx $SPHINX_THEME
}

function build {
    printf "\nBuild Gerbera Documentation\n\n"
    sphinx-build -b $SPHINX_BUILDER ../ $DOC_DIST_DIR
}

function finalize {
    deactivate
    cd $ROOT_PATH
}

function verify {
    if [ -e $DOC_DIST_DIR/index.html ]
    then
        printf "\nSUCCESS generated Gerbera documentation:\n\n"
        printf "$DOC_DIST_DIR\n\n"
    else
        printf "\nFAILED to generate Gerbera documentation\n\n"
    fi
}

## ------------------------
## MAIN
## ------------------------
create
setup
install
build
finalize
verify
