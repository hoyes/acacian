_r_         := .
_r_acacian  := ..
_r_html     := doc.html
_r_images   := ${_r_html}/pics
_r_imagesrc := images

now := ${shell date +"%Y%m%dt%H%M%S"}

exclude := .DS_Store .Trashes ._.*

tar_exclude := ${patsubst %,--exclude "%",${exclude}}

subdirs := \
	. \
	${_r_acacian}/csrc \
	${_r_acacian}/include \
	${_r_acacian}/demo \
	${_r_acacian}/xsl \

pics := Acuity-hpc.png

images := ${addprefix ${_r_images}/,${pics}}

.PHONY : all docs images

all docs :
ifeq "" "${wildcard ${_r_html}}"
	mkdir -p ${_r_html}
endif
	NaturalDocs  -s local ${addprefix -i ,${subdirs}} ${addprefix -xi ,${nd_excldirs}} -o HTML ${_r_html} -p nd

images : ${images}

${_r_images}/%.png : ${_r_imagesrc}/%.png
ifeq "${wildcard ${_r_images}}" ""
	mkdir -p ${_r_images}
endif
	cp $< $@
