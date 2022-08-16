Inspired by the VSG's, vsgopenmw-code-style aims to guide towards simplicity, modularity and in result, achieving optimal use of modern CPU architectures' caches.

* Avoid writing code. "More code equals more bugs!"
* Avoid logic. Choose simple designs that don't need logic to ensure their internal consistency. "Don't try to be smart!"
* Aim for self-contained, stateless objects with minimal dependencies so that bottlenecks could be distributed to secondary threads if required. Avoid global state and singleton objects. "First parallelize, then optimize!"
* Avoid get/set methods. Simply make self-contained class members public. Implement complex behavior by way of composition. "Prefer composition over inheritance!"
* Heed the single responsibility principle (SRP). Describe the purpose of each class. Don't extend classes beyond their described purpose. "If you have to use the word 'and' to describe a class, it's doing too much!"
* Avoid branches in inner loops (functions called many times per frame). Make use of polymorphism to eliminate several branches for the cost of one. Make use of modern C++ metaprogramming techniques to further eliminate branches. "'if' is now a bad word!"
