RPM := $(shell if [ -x "/usr/bin/rpmbuild" ]; then echo rpmbuild; \
	           else echo rpm; fi)

# Remove hyphens since they have special meaning in RPM filenames
KERNELPATH := kernel-$(subst -,,$(KERNELRELEASE))
MKSPEC     := $(srctree)/scripts/package/mkspec
PREV       := set -e; cd ..;

# rpm-pkg
# ---------------------------------------------------------------------------
$(objtree)/kernel.spec: $(MKSPEC) $(srctree)/Makefile
	$(CONFIG_SHELL) $(MKSPEC) > $@

rpm-pkg rpm: $(objtree)/kernel.spec FORCE
	$(MAKE) clean
	$(PREV) ln -sf $(srctree) $(KERNELPATH)
	$(CONFIG_SHELL) $(srctree)/scripts/setlocalversion > $(objtree)/.scmversion
	$(PREV) tar -cz $(RCS_TAR_IGNORE) -f $(KERNELPATH).tar.gz $(KERNELPATH)/.
	$(PREV) rm $(KERNELPATH)
	rm -f $(objtree)/.scmversion
	set -e; \
	$(CONFIG_SHELL) $(srctree)/scripts/mkversion > $(objtree)/.tmp_version
	set -e; \
	mv -f $(objtree)/.tmp_version $(objtree)/.version

	$(RPM) $(RPMOPTS) --target $(UTS_MACHINE) -ta ../$(KERNELPATH).tar.gz
	rm ../$(KERNELPATH).tar.gz

clean-files := $(objtree)/kernel.spec

# binrpm-pkg
# ---------------------------------------------------------------------------
$(objtree)/binkernel.spec: $(MKSPEC) $(srctree)/Makefile
	$(CONFIG_SHELL) $(MKSPEC) prebuilt > $@

binrpm-pkg: $(objtree)/binkernel.spec FORCE
	$(MAKE) KBUILD_SRC=
	set -e; \
	$(CONFIG_SHELL) $(srctree)/scripts/mkversion > $(objtree)/.tmp_version
	set -e; \
	mv -f $(objtree)/.tmp_version $(objtree)/.version

	$(RPM) $(RPMOPTS) --define "_builddir $(srctree)" --target \
		$(UTS_MACHINE) -bb $<

clean-files += $(objtree)/binkernel.spec

# Deb target
# ---------------------------------------------------------------------------
deb-pkg: FORCE
	$(MAKE) KBUILD_SRC=
	$(CONFIG_SHELL) $(srctree)/scripts/package/builddeb

clean-dirs += $(objtree)/debian/


# tarball targets
# ---------------------------------------------------------------------------
tar%pkg: FORCE
	$(MAKE) KBUILD_SRC=
	$(CONFIG_SHELL) $(srctree)/scripts/package/buildtar $@

clean-dirs += $(objtree)/tar-install/


# Help text displayed when executing 'make help'
# ---------------------------------------------------------------------------
help: FORCE
	@echo '  rpm-pkg         - Build both source and binary RPM kernel packages'
	@echo '  binrpm-pkg      - Build only the binary kernel package'
	@echo '  deb-pkg         - Build the kernel as an deb package'
	@echo '  tar-pkg         - Build the kernel as an uncompressed tarball'
	@echo '  targz-pkg       - Build the kernel as a gzip compressed tarball'
	@echo '  tarbz2-pkg      - Build the kernel as a bzip2 compressed tarball'

