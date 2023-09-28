import QtQuick 2.0
import QtQuick as QQ
import QtCore
Zzz {
    id: root
    width: height
    Rectangle {
        color: "green"
        anchors.fill: parent
        width: root.height
        height: root.foo.height

    }
    component MyRectangle: Rectangle {}
    function lala() {}
    property Rectangle foo: Rectangle{ height: 200 }
    function longfunction(a, b, c = "c", d = "d"): string {
        return "hehe: " + c + d
    }

    // documentedFunction: is documented
    // returns 'Good'
    function documentedFunction(arg1, arg2 = "Qt"): string {
        return "Good"
    }
    QQ.Rectangle {
        color:"red"
    }

    Item {
        id: someItem
        property int helloProperty
    }

    function parameterCompletion(helloWorld, helloMe: int) {
        let helloVar = 42;
        let result = someItem.helloProperty + helloWorld;
        return result;
    }

    component Base: QtObject {
        property int propertyInBase
        function functionInBase(jsParameterInBase) {
            let jsIdentifierInBase;
            return jsIdentifierInBase;
        }
    }

    Base {
        property int propertyInDerived
        function functionInDerived(jsParameterInDerived) {
            let jsIdentifierInDerived;
            return jsIdentifierInDerived;
        }

        property Base child: Base {
            property int propertyInChild
            function functionInChild(jsParameterInChild) {
                let jsIdentifierInChild;
                return someItem.helloProperty;
            }
        }
    }
    function test1() {
        {
            var helloVarVariable = 42;
        }
        // this is fine, var has no block scope
        console.log(helloVarVariable);
    }
    function test2() {
        {
            let helloLetVariable = 42;
        }
        // this is not fine, let variables have block scope
        console.log(helloLetVariable);
    }
    property var testSingleton: SystemInformation.byteOrder

    Item {
        id: itemWithEnums
        enum Hello { World }
        enum MyEnum { ValueOne, ValueTwo }
    }

    property var testEnums: itemWithEnums.World
    property var testEnums2: itemWithEnums.Hello.World

    Component.onCompleted: {}
    property var anything: Rectangle{ height: 200 }
    function createRectangle(): Rectangle {}
    function createItem(): Item {}
    function createAnything() {}
    function helloJSStatements() {
        let x = 3;
    }
    required property int requiredProperty
    readonly property int readonlyProperty: 456
    default property int defaultProperty
}
