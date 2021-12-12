Due to the brevity of this project, there is nothing particularly worth mentioning in terms of code design. Every line of code should be pretty self-explanatory.

Some quirks: the model matrix “m” is not used (set to identity). This is due to the fact that operator* has been overloaded to perform arbitrary shape transformation (refer to Shape readme for more information) and the vertex/normal data has already been transformed before sent to OpenGLShape.

After a scene file has been rendered and a new scene file is loaded, you need to uncheck “show sceneview instead” and check it again to render the new scene. This is also how the demo program behaves.
