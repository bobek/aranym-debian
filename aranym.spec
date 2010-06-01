# generic defines used by all distributions.
#
%define name	aranym
%define ver	0.9.9beta
%define _rel	1
%define copy	GPL
%define joy Petr Stehlik <pstehlik@sophics.cz>
%define group	Console/Emulators
%define realname aranym-%{ver}
%define src aranym-%{ver}.tar.gz

%define	rel	%{_rel}

# ensure where RPM thinks the docs should be matches reality
# gets around SUSE using %{_prefix}/share/doc/packages and
# fedora using %{_defaultdocdir}
#
%define	_docdir	%{_prefix}/share/doc

#
# now for distribution-specific modifications
#

# figure out which distribution we're being built on. choices so far are (open)SUSE, Mandriva and Fedora Core.
#
%define _suse	%(if [ -f /etc/SuSE-release ]; then echo 1; else echo 0; fi)
%if %{_suse}
 %define _mandriva	0
 %define _fedora	0
%else
 %define	_mandriva	%(if [ -f /etc/mandriva-release ]; then echo 1; else echo 0; fi)
 %if %{_mandriva}
  %define	_fedora		0
 %else
  %define	_fedora		%(if [ -f /etc/fedora-release ]; then echo 1; else echo 0; fi)
 %endif
%endif

# building on a (open)SUSE Linux system so make a release identifier for the (open)SUSE version
#
%if %_suse
 %define	_suse_version	%(grep VERSION /etc/SuSE-release|cut -f3 -d" ")
 %define	_suse_vernum	%(echo "%{_suse_version}"|tr -d '.')
 %define	rel		%{_rel}.suse%{_suse_vernum}
 %define	_distribution	SUSE Linux %{_suse_version}
 %define	group		System/Emulators/Other
 %define	_icondir	%{_datadir}/pixmaps/

# distro name change for SUSE >= 10.2 to openSUSE
 %if %suse_version >= 1020
  %define	_distribution	openSUSE %{_suse_version}
 %endif
Requires:	SDL >= 1.2.10
Requires:	SDL_image >= 1.2.5
BuildRequires:	SDL-devel >= 1.2.10
BuildRequires:	SDL_image-devel >= 1.2.5
BuildRequires:	update-desktop-files
%endif

# building on a Mandriva/Mandrake Linux system so use the standard Mandriva release string
#
# this is experimental and untested as yet, but should work.
#
%if %{_mandriva}
 %define	_mandriva_version	%(cat /etc/mandriva-release|cut -f4 -d" ")
 %define	_distribution		Mandriva %{_mandriva_version}
 %define	rel			%{_rel}.mdv
 %define	group			Emulators
Requires:	SDL >= 1.2.10
Requires:	SDL_image >= 1.2.5
BuildRequires:	SDL-devel >= 1.2.10
BuildRequires:	SDL_image-devel >= 1.2.5
%endif

# building on a Fedora Core Linux system. not sure if there's a release string, but create one anyway
#
# this is experimental and untested as yet, but should work.
#
%if %{_fedora}
 %define	_fedora_version		%(cat /etc/fedora-release|cut -f3 -d" ")
 %define	_distribution		Fedora Core %{_fedora_version}
 %define	rel			%{_rel}.fc%{_fedora_version}
 %define	_icondir	%{_datadir}/pixmaps/
Requires:	SDL >= 1.2.10
Requires:	SDL_image >= 1.2.5
BuildRequires:	SDL-devel >= 1.2.10
BuildRequires:	SDL_image-devel >= 1.2.5
BuildRequires:	desktop-file-utils
%endif

Name:		%{name}
Version:	%{ver}
Release:	%{rel}
License:	%{copy}
%{?_distribution:Distribution:%{_distribution}}
Summary:	32-bit Atari personal computer (Falcon030/TT030) virtual machine.
Packager:	%{joy}
URL:		http://aranym.org/
Group:		%{group}
Source0:	http://prdownloads.sourceforge.net/aranym/%{src}
BuildRoot:	/var/tmp/%{name}-root
#Patch:		%{name}-%{version}.patch

%description
ARAnyM is a software only TOS clone - a virtual machine that allows you
to run TOS, EmuTOS, FreeMiNT, MagiC and Linux-m68k operating systems
and their applications.

Authors:
Ctirad Fertr, Milan Jurik, Standa Opichal, Petr Stehlik, Johan Klockars,
Didier MEQUIGNON, Patrice Mandin and others (see AUTHORS for a full list).

%prep
%{__rm} -rf %{realname}
[ -d "%{buildroot}" ] && %{__rm} -rf %{buildroot}

%setup -q -n %{realname}
#%patch0

%build

# JIT only works on i586
#
%ifarch %ix86
 %configure --disable-nat-debug --enable-jit-compiler --enable-nfjpeg
 %{__make} depend
 %{__make}
 %{__mv} aranym aranym-jit
 %{__make} clean
%endif

%configure --disable-nat-debug --enable-addressing=direct --enable-fullmmu --enable-lilo --enable-fixed-videoram --enable-nfjpeg
%{__make} depend
%{__make}
%{__mv} aranym aranym-mmu
%{__make} clean

%configure --disable-nat-debug --enable-addressing=direct --enable-nfjpeg
%{__make} depend
%{__make}

%install
mkdir -p %{buildroot}%{_bindir}
mkdir -p %{buildroot}/%{_mandir}/man1
mkdir -p %{buildroot}/%{_datadir}/aranym
make install DESTDIR=%{buildroot}
install aranym %{buildroot}%{_bindir}
install aranym-mmu %{buildroot}%{_bindir}
install aratapif %{buildroot}%{_bindir}

# JIT only works on i586
#
%ifarch %ix86
 install aranym-jit %{buildroot}%{_bindir}
%endif

# add a desktop menu entry
#
%if %{_suse}
mkdir -p %{buildroot}/%{_icondir}
install -m644 contrib/icon-32.png %{buildroot}/%{_icondir}/aranym.png
install -D -m644 contrib/aranym.desktop %{buildroot}/%{_datadir}/applications/aranym.desktop
%suse_update_desktop_file -i aranym
%endif

%if %{_mandriva}
mkdir -p %{buildroot}%{_menudir}
cat > %{buildroot}%{_menudir}/%{name} <<EOF
?package(%{name}): \
   command="%{_bindir}/aranym" \
   icon="aranym.png" \
   title="ARAnyM" \
   longtitle="%{summary}" \
   needs="x11" \
   section="More Applications/Emulators"
EOF
%endif

%if %{_fedora}
mkdir -p %{buildroot}/%{_icondir}
install -m644 contrib/icon-32.png %{buildroot}/%{_icondir}/aranym.png
install -D -m644 contrib/aranym.desktop %{buildroot}/%{_datadir}/applications/aranym.desktop
desktop-file-install \
 --delete-original \
 --vendor fedora \
 --dir %{buildroot}%{_datadir}/applications \
 --add-category X-Fedora \
 --add-category Application \
 %{buildroot}/%{_datadir}/applications/%{name}.desktop
%endif

# Mandriva uses post-install and post-uninstall scripts for its desktop menu updates
#
%post
%if %{_mandriva}
%update_menus
%endif

%postun
%if %{_mandriva}
%clean_menus
%endif

%clean
%{__rm} -rf %{buildroot}

%files
%defattr(-,root,root)
%attr(4755,root,root) %{_bindir}/aratapif
%{_bindir}/aranym
%{_bindir}/aranym-mmu

# JIT only works on i586
#
%ifarch %ix86
 %{_bindir}/aranym-jit
%endif
%{_mandir}/man1/aranym-jit.1.gz
%{_mandir}/man1/aranym.1.gz
%{_mandir}/man1/aranym-mmu.1.gz
%{_mandir}/man1/aratapif.1.gz
%{_datadir}/aranym
%{_docdir}/aranym

# now for the desktop menu
#
%if %{_suse}
%{_icondir}/aranym.png
%attr(0644,root,root) %{_datadir}/applications/*
%endif

%if %{_fedora}
%{_icondir}/aranym.png
%attr(0644,root,root) %{_datadir}/applications/*
%endif

%if %{_mandriva}
%{_menudir}/%{name}
%endif

%changelog
* Sat Sep 05 2009 Petr Stehlik <pstehlik@sophics.cz>
New ARAnyM release.

* Sat Apr 25 2009 Petr Stehlik <pstehlik@sophics.cz>
New ARAnyM release.

* Sat Nov 08 2008 Petr Stehlik <pstehlik@sophics.cz>
New ARAnyM release.

* Wed Jan 30 2008 Petr Stehlik <pstehlik@sophics.cz>
Added SDL_image to Requires:
Bumped the minimal SDL version to 1.2.10
Enabled the Requires: for mandriva (still untested)

* Tue Jan 29 2008 Petr Stehlik <pstehlik@sophics.cz>
Desktop file corrected and renamed to lowercase.
_icondir defined for Fedora. For other changes see ChangeLog.

* Mon Jan 28 2008 Petr Stehlik <pstehlik@sophics.cz>
The right icon added. Desktop file updated. Build system root changed.
New release. Version increased. Other changes in NEWS file.

* Mon Jul 09 2007 Petr Stehlik <pstehlik@sophics.cz>
New release. Version increased. Other changes in NEWS file.

* Tue Oct 11 2006 David Bolt <davjam@davjam.org>	0.9.4beta
Added an aranym.desktop file for inclusion in desktop menus.
Temporarily uses emulator.png as the menu icon.
Added bits to spec file to try and build packages for (open)SUSE, Mandriva
and Fedora Core distributions without any changes.

* Fri Sep 22 2006 Petr Stehlik <pstehlik@sophics.cz>
New release. Version increased. Other changes in NEWS file.
Thanks to David Bolt this spec file is nicely updated - does not fail
on 64bit anymore. Thanks, David!

* Mon Feb 20 2006 Petr Stehlik <pstehlik@sophics.cz>
URL changed to aranym.org. Version increased. Other changes in NEWS file.

* Sun Apr 17 2005 Petr Stehlik <pstehlik@sophics.cz>
Files list fixed.

* Thu Apr 14 2005 Petr Stehlik <pstehlik@sophics.cz>
Version increased. NFJPEG enabled.

* Tue Feb 22 2005 Petr Stehlik <pstehlik@sophics.cz>
Version increased. aranymrc.example removed.

* Sun Feb 20 2005 Petr Stehlik <pstehlik@sophics.cz>
Version increased. LILO enabled in the MMU version.

* Sun Nov 07 2004 Petr Stehlik <pstehlik@sophics.cz>
Version increased.

* Tue Jul 06 2004 Petr Stehlik <pstehlik@sophics.cz>
Version increased.

* Mon Jul 05 2004 Petr Stehlik <pstehlik@sophics.cz>
Version increased. tools/createdisk/ removed. tools/arabridge added.
For other changes see the NEWS file.

* Sun Feb 15 2004 Petr Stehlik <pstehlik@sophics.cz>
Version increased. For other changes see the NEWS file.

* Sun Feb 08 2004 Petr Stehlik <pstehlik@sophics.cz>
Version increased. For other changes see the NEWS file.

* Wed Jan 07 2004 Petr Stehlik <pstehlik@sophics.cz>
Version increased. For other changes see the NEWS file.

* Sat Jan 03 2004 Petr Stehlik <pstehlik@sophics.cz>
font8.bmp removed.

* Sat Oct 04 2003 Petr Stehlik <pstehlik@sophics.cz>
Version increased. NFCDROM.BOS added.

* Fri Apr 11 2003 Petr Stehlik <pstehlik@sophics.cz>
Man dir fixed. Debug info disabled.

* Mon Apr 08 2003 Petr Stehlik <pstehlik@sophics.cz>
Various fixes for the 0.8.0. And full 68040 PMMU build added as aranym-mmu.
Also manual page added.

* Mon Mar 24 2003 Petr Stehlik <pstehlik@sophics.cz>
HostFS and network drivers added. ARATAPIF installed setuid root.
Ethernet enabled.

* Sun Mar 23 2003 Petr Stehlik <pstehlik@sophics.cz>
Version increased for the new release. See the NEWS file for details.

* Fri Mar 07 2003 Petr Stehlik <pstehlik@sophics.cz>
Fixed paths to share/aranym folder.

* Wed Jan 29 2003 Petr Stehlik <pstehlik@sophics.cz>
New release. Updated list of provided files.

* Tue Oct 22 2002 Petr Stehlik <pstehlik@sophics.cz>
aranym-jit (JIT compiler for m68k CPU) generated.

* Sun Oct 20 2002 Petr Stehlik <pstehlik@sophics.cz>
EmuTOS image file renamed back.

* Sat Oct 12 2002 Petr Stehlik <pstehlik@sophics.cz>
EmuTOS image file renamed. Updated for new release.

* Sun Jul 21 2002 Petr Stehlik <pstehlik@sophics.cz>
SDL GUI font and EmuTOS image files added.

* Sat Jul 20 2002 Petr Stehlik <pstehlik@sophics.cz>
Version increased.

* Thu Jun 06 2002 Petr Stehlik <pstehlik@sophics.cz>
Install path changed from /usr/local to /usr

* Mon Apr 22 2002 Petr Stehlik <pstehlik@sophics.cz>
Sound driver added

* Sun Apr 14 2002 Petr Stehlik <pstehlik@sophics.cz>
First working version
