
#include "../WinWindow/Utils/Scripting/ScriptManager.h"
#include "../WinWindow/Utils/Reflection/Reflection.h"

class GameEntity {
public:
    float x = 0, y = 0;
    int hp = 100;
    REFLECT_BODY(GameEntity)
};

BEGIN_REFLECT(GameEntity)
    REFLECT_PROPERTY(x)
    REFLECT_PROPERTY(y)
    REFLECT_PROPERTY(hp)
END_REFLECT(GameEntity)

void RunScriptTest() {
    GameEntity player;
    player.x = 0;

    ScriptManager::GetInstance()->RegisterObject("player", &player, "GameEntity");

    auto script = R"(
        x = 0;
        player.x = 50;
        
        # Predicate: When player.x > 90, set hp to 0
        player.x > 90 {
            player.hp = 0;
        }

        # Loop
        while (player.x < 100) {
            player.x = player.x + 10;
        }
    )";

    std::cout << "Executing Script...\n";
    auto id = ScriptManager::GetInstance()->ExecuteScript(script);
    
    std::cout << "Player X after script (before Update): " << player.x << "\n";
    assert(player.x >= 100);
    std::cout << "Player HP before Update: " << player.hp << "\n";
    assert(player.hp == 100);

    std::cout << "Running Update...\n";
    ScriptManager::GetInstance()->Update(0.1f);
    std::cout << "Player HP after Update: " << player.hp << "\n";
    assert(player.hp == 0);

    std::cout << "\nScript Test Passed!\n";

    ScriptManager::GetInstance()->StopScript(id);
}

struct GameManager {
    std::vector<GameEntity*> entities;

    void AddEntity(GameEntity* e) {
        entities.push_back(e);
        std::cout << "Entity added! Count: " << entities.size() << "\n";
    }

    REFLECT_BODY(GameManager)
};

BEGIN_REFLECT(GameManager)
    REFLECT_METHOD(AddEntity)
END_REFLECT(GameManager)

void RunMethodTest() {
    std::cout << "\nRunning Method and Creation Test...\n";
    ScriptManager::GetInstance();

    GameManager gm;
    ScriptManager::GetInstance()->RegisterObject("gm", &gm, "GameManager");

    auto script = R"(
        
        # Create a new entity (via factory)
        e = CreateObject("GameEntity");
        
        # Set properties
        e.hp = 555;
        e.x = 123.45;
        
        # Call method on gm to add this entity
        gm.AddEntity(e);
        
    )";

    auto id = ScriptManager::GetInstance()->ExecuteScript(script);

    assert(gm.entities.size() == 1);
    if (!gm.entities.empty()) {
        assert(gm.entities[0]->hp == 555);
        assert(gm.entities[0]->x > 123.4 && gm.entities[0]->x < 123.5);
    }
    
    ScriptManager::GetInstance()->StopScript(id);

    std::cout << "Script Method Test Passed!\n";
}

struct ComprehensiveTestObject {
    int IntVal = 0;
    float FloatVal = 0.0f;
    std::string StringVal;
    int Counter = 0;

    void Reset() {
        IntVal = 0;
        FloatVal = 0.0f;
        StringVal = "";
        Counter = 0;
    }

    void IncrementCounter() {
        Counter++;
    }

    int Add(int a, int b) {
        return a + b;
    }

    float Multiply(float a, float b) {
        return a * b;
    }

    std::string Concat(std::string a, std::string b) {
        return a + b;
    }

    void SetInt(int v) {
        IntVal = v;
    }

    int GetInt() {
        return IntVal;
    }

    REFLECT_BODY(ComprehensiveTestObject)
};

BEGIN_REFLECT(ComprehensiveTestObject)
    REFLECT_PROPERTY(IntVal)
    REFLECT_PROPERTY(FloatVal)
    REFLECT_PROPERTY(StringVal)
    REFLECT_PROPERTY(Counter)
    REFLECT_METHOD(Reset)
    REFLECT_METHOD(IncrementCounter)
    REFLECT_METHOD(Add)
    REFLECT_METHOD(Multiply)
    REFLECT_METHOD(Concat)
    REFLECT_METHOD(SetInt)
    REFLECT_METHOD(GetInt)
END_REFLECT(ComprehensiveTestObject)

void RunComprehensiveTests() {
    std::cout << "\nRunning Comprehensive Script Tests...\n";
    
    ComprehensiveTestObject testObj;
    ScriptManager::GetInstance()->RegisterObject("test", &testObj, "ComprehensiveTestObject");

   {
        std::cout << "Test 1: Arithmetic & Precedence... ";
        testObj.Reset();
        auto script = R"(
            test.IntVal = 1 + 2 * 3;     # Should be 7
            test.FloatVal = (1.0 + 2.0) * 3.0; # Should be 9.0
        )";
        auto id = ScriptManager::GetInstance()->ExecuteScript(script);
        assert(testObj.IntVal == 7);
        assert(std::abs(testObj.FloatVal - 9.0f) < 0.001f);
        std::cout << "Passed\n";

        ScriptManager::GetInstance()->StopScript(id);
    }

    {
        std::cout << "Test 2: Logic & IF... ";
        testObj.Reset();
        auto script = R"(
            # True condition
            if (1 < 2) {
                test.IntVal = 100;
            }
            # False condition
            if (1 > 2) {
                test.IntVal = 200;
            }
        )";
        auto id = ScriptManager::GetInstance()->ExecuteScript(script);
        assert(testObj.IntVal == 100);
        std::cout << "Passed\n";

        ScriptManager::GetInstance()->StopScript(id);
    }

    {
        std::cout << "Test 3: Loops (WHILE)... ";
        testObj.Reset();
        testObj.Counter = 0;
        std::string script = R"(
            while (test.Counter < 5) {
                test.IncrementCounter();
            }
        )";
        auto id = ScriptManager::GetInstance()->ExecuteScript(script);
        assert(testObj.Counter == 5);
        std::cout << "Passed\n";

        ScriptManager::GetInstance()->StopScript(id);
    }

    {
        std::cout << "Test 4: Method Calls... ";
        testObj.Reset();
        auto script = R"(
            test.IntVal = test.Add(10, 20);
            test.FloatVal = test.Multiply(2.5, 4.0);
            test.StringVal = test.Concat("Hello ", "World");
        )";
        auto id = ScriptManager::GetInstance()->ExecuteScript(script);
        assert(testObj.IntVal == 30);
        assert(std::abs(testObj.FloatVal - 10.0f) < 0.001f);
        assert(testObj.StringVal == "Hello World");
        std::cout << "Passed\n";

        ScriptManager::GetInstance()->StopScript(id);
    }

    {
        std::cout << "Test 5: Nested Logic... ";
        testObj.Reset();
        std::string script = R"(
            test.IntVal = 0;
            test.Counter = 0;
            while (test.Counter < 3) {
                if (test.Counter == 1) {
                    test.IntVal = test.IntVal + 10;
                }
                if (test.Counter == 2) {
                     test.IntVal = test.IntVal + 20;
                }
                test.IncrementCounter();
            }
        )";
        auto id = ScriptManager::GetInstance()->ExecuteScript(script);
        assert(testObj.IntVal == 30);
        std::cout << "Passed\n";

        ScriptManager::GetInstance()->StopScript(id);
    }

    {
        std::cout << "Test 6: Object Creation... ";
        GameManager gm;
        ScriptManager::GetInstance()->RegisterObject("gm_test", &gm, "GameManager");
        
        auto script = R"(
            newObj = CreateObject("ComprehensiveTestObject");
            newObj.SetInt(999);
            newObj.StringVal = "Created";
            
            # Use previously registered 'test' object to verify interaction
            test.IntVal = newObj.GetInt();
            test.StringVal = newObj.StringVal;
        )";
        
        auto id = ScriptManager::GetInstance()->ExecuteScript(script);
        assert(testObj.IntVal == 999);
        assert(testObj.StringVal == "Created");
        std::cout << "Passed\n";

        ScriptManager::GetInstance()->StopScript(id);
    }
    
    {
        std::cout << "Test 7: Predicates... ";
        testObj.Reset();
        
        auto setupScript = R"(
            test.IntVal > 50 {
                test.StringVal = "Triggered";
            }
        )";
        auto id = ScriptManager::GetInstance()->ExecuteScript(setupScript);
        
        testObj.IntVal = 60;
        ScriptManager::GetInstance()->Update(0.1f);
        
        assert(testObj.StringVal == "Triggered");
        std::cout << "Passed\n";

        ScriptManager::GetInstance()->StopScript(id);
    }

    std::cout << "All Comprehensive Tests Passed!\n";
}

void RunLoopAndDeltaTimeTest() {
    std::cout << "\nRunning Loop and DeltaTime Test...\n";
    
    ScriptManager::GetInstance()->Update(0.5f);
    assert(std::abs(ScriptManager::GetInstance()->GetDeltaTime() - 0.5f) < 0.001f);
    
    ComprehensiveTestObject testObj;
    ScriptManager::GetInstance()->RegisterObject("loopTest", &testObj, "ComprehensiveTestObject");
    testObj.Reset();

    auto script = R"(
        loop (loopTest.Counter < 3) {
            loopTest.FloatVal = GetDeltaTime();
            loopTest.IncrementCounter();
        }
    )";
    auto id = ScriptManager::GetInstance()->ExecuteScript(script);

    ScriptManager::GetInstance()->Update(0.1f);
    assert(testObj.Counter == 1);
    assert(std::abs(testObj.FloatVal - 0.1f) < 0.001f);

    ScriptManager::GetInstance()->Update(0.2f);
    assert(testObj.Counter == 2);
    assert(std::abs(testObj.FloatVal - 0.2f) < 0.001f);

    ScriptManager::GetInstance()->Update(0.1f);
    assert(testObj.Counter == 3);
    assert(std::abs(testObj.FloatVal - 0.1f) < 0.001f);

    ScriptManager::GetInstance()->Update(0.1f);
    assert(testObj.Counter == 3);

    std::cout << "Loop and DeltaTime Test Passed!\n";

    ScriptManager::GetInstance()->StopScript(id);
}

void RunExecuteFileTest() {
    std::cout << "\nRunning ExecuteFile Test...\n";
    
    auto filename = "test_script.dfs";
    {
        std::ofstream out(filename);
        out << "loopTest.IntVal = 555;"; 
    }
    
    ComprehensiveTestObject testObj;
    ScriptManager::GetInstance()->RegisterObject("loopTest", &testObj, "ComprehensiveTestObject");
    testObj.Reset();
    
    auto id = ScriptManager::GetInstance()->ExecuteFile(filename);
    
    assert(testObj.IntVal == 555);
    
    std::cout << "ExecuteFile Test Passed!\n";
    
    ScriptManager::GetInstance()->StopScript(id);
}

void RunReactiveLoopTest() {
    std::cout << "\nRunning Reactive Loop Auto-Lambda Test...\n";

    ComprehensiveTestObject testObj;
    ScriptManager::GetInstance()->RegisterObject("reactive", &testObj, "ComprehensiveTestObject");
    testObj.Reset();

    auto id = ScriptManager::GetInstance()->ExecuteScript("loop (reactive.GetInt() > 10) { reactive.IncrementCounter(); }");

    ScriptManager::GetInstance()->Update(1.0f);
    assert(testObj.Counter == 0);

    testObj.IntVal = 20;
    ScriptManager::GetInstance()->Update(1.0f);
    assert(testObj.Counter == 1);
    
    testObj.IntVal = 5;
    ScriptManager::GetInstance()->Update(1.0f);
    assert(testObj.Counter == 1);

    testObj.IntVal = 30;
    ScriptManager::GetInstance()->Update(1.0f);
    assert(testObj.Counter == 2);
    
    testObj.FloatVal = 0.0f;
    ScriptManager::GetInstance()->ExecuteScript("loop (reactive.FloatVal < 5.0) { reactive.FloatVal = reactive.FloatVal + 1.0; }");
    
    ScriptManager::GetInstance()->Update(1.0f);
    assert(testObj.Counter == 3);
    assert(std::abs(testObj.FloatVal - 1.0f) < 0.001f);
    
    std::cout << "Reactive Loop Auto-Lambda Test Passed!\n";

    ScriptManager::GetInstance()->StopScript(id);
}

void RunContextIsolationTest() {
    std::cout << "\nRunning Context Isolation Test...\n";

    ComprehensiveTestObject testObj;
    ScriptManager::GetInstance()->RegisterObject("iso", &testObj, "ComprehensiveTestObject");
    testObj.Reset();

    auto script1 = R"(
        a = 10;
        loop (iso.IntVal < a) {
            iso.IncrementCounter();
        }
    )";
    auto id1 = ScriptManager::GetInstance()->ExecuteScript(script1);

    auto script2 = R"(
        a = 5;
    )";
    auto id2 = ScriptManager::GetInstance()->ExecuteScript(script2);

    ScriptManager::GetInstance()->Update(1.0f);
    assert(testObj.Counter == 1);

    testObj.IntVal = 6;
    
    ScriptManager::GetInstance()->Update(1.0f);
    assert(testObj.Counter == 2); 

    ScriptManager::GetInstance()->StopAllScripts();
    
    std::cout << "Context Isolation Test Passed!\n";
}

void RunStopScriptsTest() {
    std::cout << "\nRunning StopAllScripts Test...\n";

    ComprehensiveTestObject testObj;
    ScriptManager::GetInstance()->RegisterObject("stopTest", &testObj, "ComprehensiveTestObject");
    testObj.Reset();

    auto id = ScriptManager::GetInstance()->ExecuteScript("loop(1==1) { stopTest.IncrementCounter(); }");
    
    ScriptManager::GetInstance()->Update(1.0f);
    assert(testObj.Counter == 1);
    
    std::cout << "Stopping script...\n";
    ScriptManager::GetInstance()->StopScript(id);
    
    ScriptManager::GetInstance()->Update(1.0f);
    assert(testObj.Counter == 1);
    
    std::cout << "StopAllScripts Test Passed!\n";
}

void RunStopSingleScriptTest() {
    std::cout << "\nRunning StopSingleScript Test (ID-based)...\n";

    ComprehensiveTestObject testObj;
    ScriptManager::GetInstance()->RegisterObject("singleStop", &testObj, "ComprehensiveTestObject");
    testObj.Reset();

    auto script1 = "loop(1==1) { singleStop.SetInt(singleStop.GetInt() + 1); }";
    auto id1 = ScriptManager::GetInstance()->ExecuteScript(script1);

    auto script2 = "loop(1==1) { singleStop.FloatVal = singleStop.FloatVal + 1.0; }";
    auto id2 = ScriptManager::GetInstance()->ExecuteScript(script2);

    ScriptManager::GetInstance()->Update(1.0f);
    assert(testObj.IntVal == 1);
    assert(std::abs(testObj.FloatVal - 1.0f) < 0.001f);
    
    std::cout << "Stopping Script 1 (ID: " << id1 << ")...\n";
    ScriptManager::GetInstance()->StopScript(id1);
    
    ScriptManager::GetInstance()->Update(1.0f);
    assert(testObj.IntVal == 1);
    assert(std::abs(testObj.FloatVal - 2.0f) < 0.001f);
    
    std::cout << "Stopping Script 2 (ID: " << id2 << ")...\n";
    ScriptManager::GetInstance()->StopScript(id2);
    
    ScriptManager::GetInstance()->Update(1.0f);
    assert(testObj.IntVal == 1);
    assert(std::abs(testObj.FloatVal - 2.0f) < 0.001f);

    std::cout << "StopSingleScript Test Passed!\n";
}


void RunStressTest() {
    std::cout << "\nRunning Heavy Stress Test...\n";
    
    ComprehensiveTestObject testObj;
    ScriptManager::GetInstance()->RegisterObject("stress", &testObj, "ComprehensiveTestObject");
    
    auto script = R"(
        # ==========================================
        # 1. Fibonacci Sequence (Logic & Variables)
        # ==========================================
        f0 = 0;
        f1 = 1;
        count = 0;
        
        # Generate 10th fib number
        while (count < 10) {
            temp = f0 + f1;
            f0 = f1;
            f1 = temp;
            count = count + 1;
        }
        
        stress.IntVal = f1; # Should be 89 (0,1,1,2,3,5,8,13,21,34,55,89 is 11th? wait. 
                            # Start: 0, 1. 
                            # 1: t=1, f0=1, f1=1. 
                            # 2: t=2, f0=1, f1=2.
                            # 3: t=3, f0=2, f1=3.
                            # 4: t=5, f0=3, f1=5.
                            # 5: t=8, f0=5, f1=8.
                            # 6: t=13, f0=8, f1=13.
                            # 7: t=21, f0=13, f1=21.
                            # 8: t=34, f0=21, f1=34.
                            # 9: t=55, f0=34, f1=55.
                            # 10: t=89, f0=55, f1=89.
                            # Result 89.

        # ==========================================
        # 2. Complex Arithmetic (Precedence & Types)
        # ==========================================
        val_a = 100;
        val_b = 2.5;
        val_c = 4;
        
        # (100 * 2.5) / 4 - 50 + (10 % 3? No mod yet) -> (250 / 4) - 50 = 62.5 - 50 = 12.5
        # Parentheses test
        result = (val_a * val_b) / val_c - 50; 
        
        # Float + Int interaction
        result = result + 0.5; # 13.0
        
        stress.FloatVal = result; 

        # ==========================================
        # 3. String Manipulation Loop
        # ==========================================
        str_res = "S";
        k = 0;
        
        while (k < 10) {
            # Branching inside loop
            if (k < 3) {
                str_res = str_res + "a";
            }
            if (k > 7) {
                str_res = str_res + "z";
            }
            if (k == 5) {
                str_res = str_res + "-";
            }
            k = k + 1;
        }
        # k=0: <3 -> Sa
        # k=1: <3 -> Saa
        # k=2: <3 -> Saaa
        # k=3: - 
        # k=4: -
        # k=5: ==5 -> Saaa-
        # k=6: -
        # k=7: -
        # k=8: >7 -> Saaa-z
        # k=9: >7 -> Saaa-zz
        
        stress.StringVal = str_res;

        # ==========================================
        # 4. Method Calls in Expressions
        # ==========================================
        
        # stress.Add(10, 20) -> 30
        # stress.Multiply(2, 2) -> 4
        # 30 + 4 = 34
        
        stress.Counter = stress.Add(10, 20) + stress.Multiply(2.0, 2.0);
        
    )";
    
    std::cout << "Executing Stress Script...\n";
    ScriptManager::GetInstance()->ExecuteScript(script);
    
    std::cout << "Fib Result (IntVal): " << testObj.IntVal << "\n";
    assert(testObj.IntVal == 89);
    
    std::cout << "Math Result (FloatVal): " << testObj.FloatVal << "\n";
    assert(std::abs(testObj.FloatVal - 13.0f) < 0.001f);
    
    std::cout << "String Result (StringVal): " << testObj.StringVal << "\n";
    assert(testObj.StringVal == "Saaa-zz");
    
    std::cout << "Method Expression Result (Counter): " << testObj.Counter << "\n";
    assert(testObj.Counter == 34);

    std::cout << "Stress Test Passed!\n\n\n";
}


struct BaseA {
    int ValA = 10;
    REFLECT_BODY(BaseA)
};
BEGIN_REFLECT(BaseA)
    REFLECT_PROPERTY(ValA)
END_REFLECT(BaseA)

struct BaseB {
    float ValB = 20.5f;
    REFLECT_BODY(BaseB)
};
BEGIN_REFLECT(BaseB)
    REFLECT_PROPERTY(ValB)
END_REFLECT(BaseB)

struct Derived : public BaseA, public BaseB {
    std::string ValD = "Derived";
    REFLECT_BODY(Derived)
};
BEGIN_REFLECT(Derived, BaseA, BaseB)
    REFLECT_PROPERTY(ValD)
END_REFLECT(Derived)

void RunInheritanceTest() {
    std::cout << "\nRunning Inheritance Test (Multiple Bases)...\n";
    Derived d;
    ScriptManager::GetInstance()->RegisterObject("d", &d, "Derived");
    
    auto script = R"(
        d.ValA = 100;
        d.ValB = 200.5;
        d.ValD = "Modified";
    )";
    ScriptManager::GetInstance()->ExecuteScript(script);
    
    assert(d.ValA == 100);
    assert(std::abs(d.ValB - 200.5f) < 0.001f);
    assert(d.ValD == "Modified");
    std::cout << "Inheritance Test Passed!\n";
}

void RunScriptTests() {
    RunScriptTest();
    RunMethodTest();
    RunComprehensiveTests();
    RunStressTest();
    RunInheritanceTest();
    RunLoopAndDeltaTimeTest();
    RunExecuteFileTest();
    RunReactiveLoopTest();
    RunContextIsolationTest();
    RunStopScriptsTest();
    RunStopSingleScriptTest();
}