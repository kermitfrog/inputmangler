workspace.clientActivated.connect(function(client) {
  if (client == null)
  {
    print("client == null")
    return
  }
  else if (client.caption == null)
  {
    print("client == null")
    return
  }

  print(client.caption + "is activated")
  callDBus("org.inputManglerInterface", 
	     "/org/inputMangler/Interface",
	     "org.inputMangler.API.Interface",
	     "activeWindowChanged")//,

  client.captionChanged.connect(function() {
      callDBus("org.inputManglerInterface", 
	     "/org/inputMangler/Interface",
	     "org.inputMangler.API.Interface",
	     "activeWindowTitleChanged")
  })
});

