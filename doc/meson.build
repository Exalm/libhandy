if get_option('gtk_doc')

subdir('xml')

private_headers = [
    'config.h',
    'gconstructorprivate.h',
    'gtkprogresstrackerprivate.h',
    'gtk-window-private.h',
    'hdy-animation-private.h',
    'hdy-carousel-box-private.h',
    'hdy-main-private.h',
    'hdy-keypad-button-private.h',
    'hdy-preferences-group-private.h',
    'hdy-preferences-page-private.h',
    'hdy-shadow-helper-private.h',
    'hdy-stackable-box-private.h',
    'hdy-swipe-tracker-private.h',
    'hdy-swipeable-private.h',
    'hdy-view-switcher-button-private.h',
]

images = [
  'images/dialog-desktop.png',
  'images/dialog-mobile.png',
  'images/header-bar.png',
  'images/keypad.png',
  'images/list.png',
  'images/preferences-window.png',
  'images/search.png',
  'images/view-switcher.png',
  'images/view-switcher-bar.png',
]

content_files = [
  'build-howto.xml',
  'hdy-migrating-0-0-to-1.xml',
  'visual-index.xml',
]

glib_prefix  = dependency('glib-2.0').get_pkgconfig_variable('prefix')
glib_docpath = glib_prefix / 'share' / 'gtk-doc' / 'html'
docpath      = get_option('datadir') / 'gtk-doc' / 'html'

gnome.gtkdoc('libhandy',
           main_xml: 'handy-docs.xml',
            src_dir: [
                       meson.source_root() / 'src',
                       meson.build_root()  / 'src',
                     ],
       dependencies: libhandy_dep,
  gobject_typesfile: 'libhandy.types',
          scan_args: [
                       '--rebuild-types',
                       '--ignore-headers=' + ' '.join(private_headers),
                     ],
       fixxref_args: [
                       '--html-dir=@0@'.format(docpath),
                       '--extra-dir=@0@'.format(glib_docpath / 'glib'),
                       '--extra-dir=@0@'.format(glib_docpath / 'gobject'),
                       '--extra-dir=@0@'.format(glib_docpath / 'gio'),
                       '--extra-dir=@0@'.format(glib_docpath / 'gi'),
                       '--extra-dir=@0@'.format(glib_docpath / 'gtk3'),
                     ],
        install_dir: 'libhandy',
      content_files: content_files,
        html_assets: images,
            install: true)

endif
