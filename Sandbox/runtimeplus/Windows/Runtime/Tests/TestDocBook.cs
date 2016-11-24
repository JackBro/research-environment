using System;
using System.Xml;
using System.Xml.Serialization;
namespace Refly.Cons
{
	using NDocBook;
	/// <summary>
	/// Summary description for TestDocBook.
	/// </summary>
	public class TestDocBook
	{
		static public void Test()
		{

			Book b = new Book();
			b.Title = new Title();
			b.Title.Text.AddString("My Title");

			b.Items.AddPart(new Part());
			

			XmlSerializer ser = new XmlSerializer(typeof(Book));
			XmlTextWriter writer =new XmlTextWriter(Console.Out);
			writer.Formatting = Formatting.Indented;
			ser.Serialize(writer,b);

	
		}
	}
}
