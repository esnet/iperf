esnet-gh-pages-base
===================

Base templates for ESnet's GitHub pages. These pages are created using the
Sphinx_ documentation package using the sphinx-bootstrap-theme_ with some
pages.  This repo is meant to be included into a project using git subtree and
provides the overrides and customizations to the base theme.

.. _Sphinx: http://sphinx-doc.org
.. _sphinx-bootstrap-theme: https://github.com/ryan-roemer/sphinx-bootstrap-theme

Installation
------------

1. Install Sphinx and sphinx-bootstrap-theme. See the instructions below for
   installing these either using the Mac OS X base system python or MacPorts.
2. ``cd $PROJECT_ROOT``
3. ``mkdir docs``
4. ``git subtree add --prefix docs/_esnet https://github.com/esnet/esnet-gh-pages-base.git master --squash``
5. ``cd docs``
6. ``sphinx-quickstart``
7. ``ln -s ../_esnet/static _static/esnet``
8. edit ``conf.py`` as described in the next section
  
Editing conf.py
^^^^^^^^^^^^^^^

``sphinx-quickstart`` creates a basic conf.py file, however to use the ESnet
theme we need to make some changes. Make the following changes to conf.py::

   # add this with the imports at the top of the file
   import sphinx_bootstrap_theme

   # change templates_path to this
   templates_path = ['_esnet/templates']

   # add _esnet to exclude_patterns
   exclude_patterns = ['_build', '_esnet']

   # change html_theme and html_theme_path:
   html_theme = 'bootstrap'
   html_theme_path = sphinx_bootstrap_theme.get_html_theme_path()

   # add html_theme options:
   html_theme_options = {
          "navbar_pagenav": False,
          "nosidebar": False,
          "navbar_class": "navbar",
          "navbar_site_name": "Section",
          "source_link_position": "footer",
       "navbar_links": [
           ("Index", "genindex"),
           ("ESnet", "https://www.es.net", True),
       ],
   }

   # add html_logo and html_sidebars
   html_logo = "_esnet/static/logo-esnet-ball-sm.png"
   html_sidebars = {'index': None, 'search': None, '*': ['localtoc.html']}
   html_favicon = "_esnet/static/favicon.ico"
   html_context = {
      "github_url": "https://github.com/esnet/PROJNAME",
   }

That's it!

Sphinx Installation using Mac OS X Base Python
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

1. sudo /usr/bin/easy_install pip
2. sudo /usr/local/bin/pip install sphinx sphinx-bootstrap-theme

Sphinx Installation using MacPorts
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

1. port install python27 py27-pip py27-sphinx
2. port select pip py27-pip
3. port select sphinx py27-sphinx
4. pip install sphinx sphinx-bootstrap-theme # make sure this is
   /opt/local/bin/pip

Creating Content, Previewing and Publishing
-------------------------------------------

The files are in the ``docs`` directory.  Take a look at the content of
``index.rst``.  Take a look at the docs from other projects and review the
documentation for Sphinx_.

Building HTML
^^^^^^^^^^^^^

In the ``docs`` directory run ``make clean html``.

Previewing the site
^^^^^^^^^^^^^^^^^^^

``open _build/html/index.html``

or

``open -a /Application/Google\ Chrome.app _build/html/index.html``

Publishing the site
^^^^^^^^^^^^^^^^^^^

From the ``docs`` directory run ``_esnet/deploy.sh``.  It will be visible at:
``http://github.com/esnet/PROJECT``.
