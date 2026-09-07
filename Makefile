VERSION ?= dev
LANGUAGE ?= english
RELEASE_ROOT ?= releases
PLATFORM ?= all

.PHONY: stm wave release release-stm release-wave flash-stm flash-wave generate-wordlists test audit clean

generate-wordlists:
	python3 tools/generate_wordlists.py

stm:
	$(MAKE) -C stm32

wave:
	$(MAKE) -C waveshare

release:
	@set -e; \
	version="$(VERSION)"; \
	platform="$(PLATFORM)"; \
	case "$$version" in \
		""|.|..|*/*) echo "Invalid VERSION: $$version" >&2; exit 1 ;; \
	esac; \
	case "$$platform" in \
		all|stm32|waveshare) ;; \
		*) echo "Invalid PLATFORM: $$platform (use all, stm32, or waveshare)" >&2; exit 1 ;; \
	esac; \
	if [ "$$platform" = all ] || [ "$$platform" = stm32 ]; then \
		$(MAKE) stm; \
	fi; \
	if [ "$$platform" = all ] || [ "$$platform" = waveshare ]; then \
		$(MAKE) wave; \
	fi; \
	release_dir="$(RELEASE_ROOT)/$$version"; \
	rm -rf "$$release_dir"; \
	mkdir -p "$$release_dir"; \
	if [ "$$platform" = all ] || [ "$$platform" = stm32 ]; then \
		for language in english czech french italian portuguese spanish; do \
			cp "stm32/build/$$language/bip39-stm32-$$language.bin" \
				"$$release_dir/stm32-$$language-$$version.bin"; \
		done; \
	fi; \
	if [ "$$platform" = all ] || [ "$$platform" = waveshare ]; then \
		for language in english czech french italian portuguese spanish; do \
			if [ "$$language" = english ]; then \
				target=bip39_waveshare; \
			else \
				target=bip39_waveshare_$$language; \
			fi; \
			cp "waveshare/build/$$target.uf2" \
				"$$release_dir/waveshare-$$language-$$version.uf2"; \
		done; \
	fi; \
	if command -v shasum >/dev/null 2>&1; then \
		(cd "$$release_dir" && shasum -a 256 * > SHA256SUMS); \
	else \
		(cd "$$release_dir" && sha256sum * > SHA256SUMS); \
	fi; \
	echo "Release created in $$release_dir"; \
	echo "Checksums written to $$release_dir/SHA256SUMS"

release-stm:
	$(MAKE) -C stm32 release VERSION=$(VERSION)

release-wave:
	$(MAKE) -C waveshare release VERSION=$(VERSION)

flash-stm:
	$(MAKE) -C stm32 flash WORDLIST=$(LANGUAGE)

flash-wave:
	$(MAKE) -C waveshare flash LANGUAGE=$(LANGUAGE)

test:
	$(MAKE) -C stm32 test
	$(MAKE) -C waveshare test

audit:
	$(MAKE) -C stm32 audit
	$(MAKE) -C waveshare audit

clean:
	$(MAKE) -C stm32 clean
	$(MAKE) -C waveshare clean
