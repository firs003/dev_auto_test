exclude_dirs:=inc out backup .git

#commom must be compiled before any other modules
dirs:=src

subdir=$(shell find . -maxdepth 1 -type d)
dirs+=$(filter-out ./src,$(subdir))
dirs:=$(basename $(patsubst ./%,%,$(dirs)))
dirs:=$(filter-out $(exclude_dirs),$(dirs))

.PHONY: $(dirs) clean

$(dirs):
	@echo "dirs=$(dirs)"
	@for dir in $(dirs); do \
		$(MAKE) -C $$dir; \
	done

clean:
	@for dir in $(dirs); do \
		$(MAKE) -C $$dir clean; \
	done
