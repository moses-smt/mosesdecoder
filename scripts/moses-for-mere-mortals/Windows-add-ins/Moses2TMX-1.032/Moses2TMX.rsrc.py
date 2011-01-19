{'application':{'type':'Application',
          'name':'Moses2TMX',
    'backgrounds': [
    {'type':'Background',
          'name':'bgMoses2TMX',
          'title':u'Moses2TMX-1.032',
          'size':(277, 307),
          'statusBar':1,

        'menubar': {'type':'MenuBar',
         'menus': [
             {'type':'Menu',
             'name':'menuFile',
             'label':u'&File',
             'items': [
                  {'type':'MenuItem',
                   'name':'menuFileSelectDirectory',
                   'label':u'Select &Directory ...\tAlt+D',
                  },
                  {'type':'MenuItem',
                   'name':'menuFileCreateTMXFiles',
                   'label':u'&Create TMX Files\tAlt+C',
                  },
                  {'type':'MenuItem',
                   'name':'Sep1',
                   'label':u'-',
                  },
                  {'type':'MenuItem',
                   'name':'menuFileExit',
                   'label':u'&Exit\tAlt+E',
                  },
              ]
             },
             {'type':'Menu',
             'name':'menuHelp',
             'label':u'&Help',
             'items': [
                  {'type':'MenuItem',
                   'name':'menuHelpShowHelp',
                   'label':u'&Show Help\tAlt+S',
                  },
              ]
             },
         ]
     },
         'components': [

{'type':'Button', 
    'name':'btnSelectDirectory', 
    'position':(15, 15), 
    'size':(225, 25), 
    'font':{'faceName': u'Arial', 'family': 'sansSerif', 'size': 10}, 
    'label':u'Select Directory ...', 
    },

{'type':'StaticText', 
    'name':'StaticText3', 
    'position':(17, 106), 
    'font':{'faceName': u'Arial', 'family': 'sansSerif', 'size': 10}, 
    'text':u'Target Language:', 
    },

{'type':'ComboBox', 
    'name':'cbStartingLanguage', 
    'position':(18, 75), 
    'size':(70, -1), 
    'items':[], 
    },

{'type':'ComboBox', 
    'name':'cbDestinationLanguage', 
    'position':(17, 123), 
    'size':(70, -1), 
    'items':[u'DE-PT', u'EN-PT', u'ES-PT', u'FR-PT'], 
    },

{'type':'Button', 
    'name':'btnCreateTMX', 
    'position':(20, 160), 
    'size':(225, 25), 
    'font':{'faceName': u'Arial', 'family': 'sansSerif', 'size': 10}, 
    'label':u'Create TMX Files', 
    },

{'type':'StaticText', 
    'name':'StaticText1', 
    'position':(18, 56), 
    'font':{'faceName': u'Arial', 'family': 'sansSerif', 'size': 10}, 
    'text':u'Source Language:', 
    },

] # end components
} # end background
] # end backgrounds
} }
