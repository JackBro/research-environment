using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using NUnit.Framework;

namespace unittest.MyProject
{
    [TestFixture]
    public class Test1
    {
        [Test]
        public void Test01()
        {
            Assert.IsTrue(true);
            Assert.Fail("Sample failing test case");
        }
    }
}
