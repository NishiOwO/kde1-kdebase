# Building KDE1 for FreeBSD
## 1. Build qt1
Requirements: `xorg` `gmake` `gcc`
1. `git clone https://github.com/nishiowo/qt1 /opt/qt1 && cd /opt/qt1`
2. `mkdir lib && gmake freebsd-g++-shared`
3. Run `gmake`, works with -j building
4. `ln -s /opt/qt1/bin/moc /usr/bin/moc-qt1`
5. Create `/usr/local/libdata/ldconfig/qt1` with `/opt/qt1/lib`, then restart `ldconfig`
## 2. Build kde1-kdelibs
Requirements: `cmake` `jpeg-turbo`(or any jpeg) `tiff` `gettext`
1. `git clone https://github.com/nishiowo/kde1-kdelibs && cd kde1-kdelibs`
2. `mkdir build && cd build`
3. `cmake -DCMAKE_INSTALL_PREFIX=/usr/kde1 -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ ..`
4. Run `gmake`, works with -j building
5. `gmake install` with root
6. Create `/usr/local/libdata/ldconfig/kde1` with `/usr/kde1/lib`, then restart `ldconfig`
## 3. Build kde1-kdebase
Requirements: `libGLU`
Optional: `pulseaudio`
1. `git clone https://github.com/nishiowo/kde1-kdebase && cd kde1-kdebase`
2. `mkdir build && cd build`
3. `cmake -DCMAKE_INSTALL_PREFIX=/usr/kde1 -DCMAKE_INSTALL_PREFIX=/usr/kde1 -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ ..`
4. Run `gmake`, works with -j building
5. `gmake install` with root
6. `chmod ugo+rx /usr/kde1/bin/*` with root

## Notes
You need load `pty` module to get konsole working
