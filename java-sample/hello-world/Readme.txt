javac ./HelloWorld.java    #success, Message.class and HelloWorld.class created

rm HelloWorld.class
rm Message.class

cd ..
javac ./hello-world/HelloWorld.java    #failed
==================================================
why?
Because, java cannot find the Message.java without classpath!
==================================================
javac -cp ./hello-world ./hello-world/HelloWorld.java    #success!
java -cp ./hello-world HelloWorld    #success!

#use ':' to seperate possible path to be searched.(';' in Windows instead)
java -cp ".:./hello-world" HelloWorld
