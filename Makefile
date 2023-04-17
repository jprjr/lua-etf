.PHONY: release clean github-release

PKGCONFIG = pkg-config
CFLAGS = -Wall -Wextra -g -O2 -fPIC
LDFLAGS =

LUA=lua

CFLAGS += $(shell $(PKGCONFIG) --cflags $(LUA))

VERSION = $(shell LUA_CPATH="./csrc/?.so" lua -e 'print(require("etf")._VERSION)')

lib: csrc/etf.so

csrc/etf.so: csrc/etf.c
	$(CC) -shared $(CFLAGS) -o $@ $^ $(LDFLAGS)

github-release: lib
	source $(HOME)/.github-token && github-release release \
	  --user jprjr \
	  --repo lua-etf \
	  --tag v$(VERSION)
	source $(HOME)/.github-token && github-release upload \
	  --user jprjr \
	  --repo lua-etf \
	  --tag v$(VERSION) \
	  --name lua-etf-$(VERSION).tar.gz \
	  --file dist/lua-etf-$(VERSION).tar.gz
	source $(HOME)/.github-token && github-release upload \
	  --user jprjr \
	  --repo lua-etf \
	  --tag v$(VERSION) \
	  --name lua-etf-$(VERSION).tar.xz \
	  --file dist/lua-etf-$(VERSION).tar.xz

release: lib
	rm -rf dist/lua-etf-$(VERSION)
	rm -rf dist/lua-etf-$(VERSION).tar.gz
	rm -rf dist/lua-etf-$(VERSION).tar.xz
	mkdir -p dist/lua-etf-$(VERSION)/csrc
	rsync -a --exclude='*.so' csrc/ dist/lua-etf-$(VERSION)/csrc/
	rsync -a LICENSE dist/lua-etf-$(VERSION)/LICENSE
	rsync -a README.md dist/lua-etf-$(VERSION)/README.md
	sed 's/@VERSION@/$(VERSION)/g' < etf-template.rockspec > dist/lua-etf-$(VERSION)/etf-$(VERSION)-1.rockspec
	cd dist && tar -c -f lua-etf-$(VERSION).tar lua-etf-$(VERSION)
	cd dist && gzip -k lua-etf-$(VERSION).tar
	cd dist && xz lua-etf-$(VERSION).tar

clean:
	rm -f csrc/etf.so
