var activeclient;

function onCaptionChange() {
  try {
        callDBus("org.inputManglerInterface", 
	     "/org/inputMangler/Interface",
	     "org.inputMangler.API.Interface",
	     "activeWindowTitleChanged", activeclient.caption)
  }
  catch(e) {
  }
}

workspace.clientActivated.connect(function(client) {
  if (client == null)
  {
    return
  }
  else if (client.caption == null)
  {
    return
  }
  if (activeclient != null) {
    try {
      activeclient.captionChanged.disconnect(onCaptionChange)
    }
    catch(e) {
    }
  }
  activeclient = client;
//  print("class = " + client.resourceClass)
//  print(client.caption + " is activated")
  try {
    callDBus("org.inputManglerInterface", 
	     "/org/inputMangler/Interface",
	     "org.inputMangler.API.Interface",
	     "activeWindowChanged", client.resourceClass, client.caption)
  }
  catch(e) {
    
  }

  client.captionChanged.connect(onCaptionChange)
});
