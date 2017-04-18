https://maven.apache.org/guides/getting-started/maven-in-five-minutes.html

>mvn archetype:generate -DgroupId=com.mycompany.app -DartifactId=my-app -DarchetypeArtifactId=maven-archetype-quickstart -DinteractiveMode=false

    my-app
    |-- pom.xml
    `-- src
        |-- main
        |   `-- java
        |       `-- com
        |           `-- mycompany
        |               `-- app
        |                   `-- App.java
        `-- test
            `-- java
                `-- com
                    `-- mycompany
                        `-- app
                            `-- AppTest.java

You executed the Maven goal archetype:generate, belonging to the plugin archetype.
A plugin is a collection of goals with a general common purpose.

>cd ./my-app
>mvn package

Build lifecycle:
    validate
    compile
    test
    package
    verify
    install
    deploy

>java -cp target/my-app-1.0-SNAPSHOT.jar com.mycompany.app.App
