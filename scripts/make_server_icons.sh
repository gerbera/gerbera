#!/bin/bash

# run as
# make_icons-mediatomb.sh mylogo.jpg
# Will generate all the required mediatomb icons

src="${1}"


convert "${src}" -filter Lanczos -resize x32 -background black -flatten bmp:"mt-icon32.bmp"
convert "${src}" -filter Lanczos -resize x32 -background black -flatten png:"mt-icon32.png"
convert "${src}" -filter Lanczos -resize x32 -background black -flatten jpg:"mt-icon32.jpg"

convert "${src}" -filter Lanczos -resize x48 -background black -flatten bmp:"mt-icon48.bmp"
convert "${src}" -filter Lanczos -resize x48 -background black -flatten png:"mt-icon48.png"
convert "${src}" -filter Lanczos -resize x48 -background black -flatten jpg:"mt-icon48.jpg"

convert "${src}" -filter Lanczos -resize x120 -background black -flatten bmp:"mt-icon120.bmp"
convert "${src}" -filter Lanczos -resize x120 -background black -flatten png:"mt-icon120.png"
convert "${src}" -filter Lanczos -resize x120 -background black -flatten jpg:"mt-icon120.jpg"
