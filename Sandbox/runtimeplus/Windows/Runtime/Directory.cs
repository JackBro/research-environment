using System;
using System.IO;
using System.Xml;
using System.Xml.Linq;
using System.Xml.XPath;
using System.Data;


namespace System.Xml.Directory
{
   /// <summary>
   /// This class generates a list of files in a directory as XML.  It can
   /// return the XML as a string, a W3C DOM document, or a DataSet.  I
   /// </summary>
   public class XmlDirectoryLister 
   {
	  private Boolean m_bUseDomCalls;

	  /// <summary>
	  /// Constructor
	  /// </summary>     
      /// <param name="usedomcalls">Use W3C DOM calls to build XML?</param>
	  /// <returns>None</returns>
	  public XmlDirectoryLister(Boolean usedomcalls)
	  {
          try
          {
              m_bUseDomCalls = usedomcalls;
          }
          catch (Exception)
          {
              
              throw;
          }
	  }
	  
	  /// <summary>
	  /// This method generates a list of files in a directory as XML and
	  /// returns the XML as a string.
	  /// </summary>
      /// <example>  
      /// This sample shows how to call the <see cref="GetXmlString"/> method.
      /// <code> 
      /// class TestClass  
      /// { 
      ///     static int Main()  
      ///     { 
      ///         string xmldir = @"c:\temp";
      ///         XmlDirectoryLister xmlString = new XmlDirectoryLister(true);
      ///         xmlString.GetXmlString(xmldir); 
      ///     } 
      /// } 
      /// </code> 
      /// </example> 
	  /// <param name="strDirectory">List files in this directory</param>
	  /// <returns>A XML string with the directory file list</returns>
	  public string GetXmlString(string strDirectory)
	  {
		 string StrXML = "";
	  
		 XmlDocument doc = GetXml(strDirectory);
		 if (doc != null)
		 {
			  StringWriter writer = new StringWriter();
			  doc.Save(writer);
			  StrXML = writer.ToString();
		 }
		 
		 doc = null;
		 return StrXML;
	  }
	   

	  /// <summary>
	  /// This method generates a list of files in a directory as XML and
	  /// returns the XML as a DataSet.
	  /// </summary>
	  /// <param name="strDirectory">List files in this directory</param>
	  /// <returns>A DataSet with the directory file list</returns>
	  public DataSet GetXmlDataset(string strDirectory)
	  {
          try
          {
              DataSet ds = null;
              string StrXml = GetXmlString(strDirectory);
              //XmlDataDocument datadoc = new XmlDataDocument();
              XDocument xmlfile = XDocument.Load(StrXml);
              XmlReader reader = xmlfile.CreateReader();
              ds.ReadXml(new StringReader(StrXml));

              return ds;
          }
          catch (Exception)
          {
              
              throw;
          }
	  }
	   

	  /// <summary>
	  /// This public method generates a list of files in a directory as XML and
	  /// returns the XML as a W3C DOM document.
	  /// </summary>
	  /// <param name="strDirectory">List files in this directory</param>
	  /// <returns>A W3C DOM docuemnt with the directory file list</returns>
	   public XmlDocument GetXmlDocument(string strDirectory)
	  {
          try
          {
              return GetXml(strDirectory);
          }
          catch (Exception)
          {
              
              throw;
          }
	  }
      /// <summary>
      /// This public method get a list of files in a directory as XML and
      /// returns the XML as XML document.
      /// </summary>
      /// <param name="strDirectory">List files in this directory</param>
       /// <returns>A an XML Document</returns>
	  public XmlDocument GetXml(string strDirectory)
	  {
          try
          {
              XmlDocument XmlDoc;
              if (m_bUseDomCalls)
                  XmlDoc = GetXmlUsingDomCalls(strDirectory);
              else
                  XmlDoc = GetXmlUsingTextWriterClass(strDirectory);

              return XmlDoc;
          }
          catch (Exception)
          {
              
              throw;
          }
	  }

	  /// <summary>
	  /// This private method generates a list of files in a directory as XML and
	  /// returns the XML as a W3C DOM document using the DOM calls.
	  /// </summary>
	  /// <param name="strDirectory">List files in this directory</param>
	  /// <returns>A W3C DOM docuemnt with the directory file list</returns>
	  public static XmlDocument GetXmlUsingDomCalls( string strDirectory )
	  {
		 // Create the document.
		 XmlDocument doc = new XmlDocument();

		 // Insert the xml processing instruction and the root node
		 XmlDeclaration dec = 
			doc.CreateXmlDeclaration("1.0", "", "yes");
		 doc.PrependChild ( dec );

		 // Add the root element
		 XmlElement nodeElem = 
			doc.CreateElement( "dirlist" );
		 doc.AppendChild( nodeElem );
		 
		 Boolean bFirst = true;

		 // Process the directory list
		 DirectoryInfo dir = new DirectoryInfo( strDirectory );
		 foreach ( FileSystemInfo entry in dir.GetFileSystemInfos() ) 
		 {
			if ( bFirst == true )
			{
			   // If we haven't added any elements yet, go ahead and add a text element which
			   // contains the full directory path.
			   XmlElement root = doc.DocumentElement;

			   String strFullName = entry.FullName;
			   String strFileName = entry.Name;
		 
			   String strDir = strFullName.Substring( 0, strFullName.Length - strFileName.Length );
			   root.SetAttribute ("dir", strDir );

			   bFirst = false;
			}   
			
			// Add a new text node with a tag entry.  There will be one added per
			// item encountered in the directory.
			XmlElement elem = doc.CreateElement("entry");
			doc.DocumentElement.AppendChild(elem);
			
			// Write out the things we are interested in about this entry in the
			// directory.
			addTextElement( doc, elem, "name", entry.Name );
			addTextElement( doc, elem, "created", entry.CreationTime.ToString() );
			addTextElement( doc, elem, "lastaccess", entry.LastAccessTime.ToString() );
			addTextElement( doc, elem, "lastwrite", entry.LastWriteTime.ToString() );
			addTextElement( doc, elem, "isfile", ( (entry.Attributes & FileAttributes.Directory) > 0 ) ? "False" : "True" );
			addTextElement( doc, elem, "isdir", ( (entry.Attributes & FileAttributes.Directory) > 0 ) ? "True" : "False" );
			addTextElement( doc, elem, "readonly", ( (entry.Attributes & FileAttributes.ReadOnly) > 0 ) ? "True" : "False" );
		 }
			   
		 return doc;
	  } 
	   

	  /// <summary>
	  /// This private method adds a text element to the XML document as the last
	  /// child of the current element.
	  /// </summary>
	  /// <param name="doc">The XML document</param>
	  /// <param name="nodeParent">Parent of the node we are adding</param>
	  /// <param name="strTag">The tag of the element to add</param>
	  /// <param name="strValue">The text value of the new element</param>
	  public static void addTextElement( XmlDocument doc, XmlElement nodeParent, string strTag, string strValue )
	  {
          try
          {
              XmlElement nodeElem = doc.CreateElement(strTag);
              XmlText nodeText = doc.CreateTextNode(strValue);
              nodeParent.AppendChild(nodeElem);
              nodeElem.AppendChild(nodeText);
          }
          catch (Exception)
          {
              
              throw;
          }
	  } 


	  /// <summary>
	  /// This private method generates a list of files in a directory as XML and
	  /// returns the XML as a W3C DOM document using the XmlTextWriter class.
	  /// </summary>
	  /// <param name="strDirectory">List files in this directory</param>
	  /// <returns>A W3C DOM docuemnt with the directory file list</returns>
	  public static XmlDocument GetXmlUsingTextWriterClass( string strDirectory )
	  {
		 StringWriter writerString = new StringWriter();
		
		 XmlTextWriter writer = new XmlTextWriter( writerString );
		 
		 writer.WriteStartDocument(true);
		 
		 Boolean bFirst = true;
	  
		 // Process the directory list
		 DirectoryInfo dir = new DirectoryInfo( strDirectory );
		 foreach ( FileSystemInfo entry in dir.GetFileSystemInfos() ) 
		 {
			
			if ( bFirst == true )
			{
			   // If we haven't added any elements yet, go ahead and add a text element which
			   // contains the full directory path.
			   writer.WriteStartElement( "dirlist", "" );

			   String strFullName = entry.FullName;
			   String strFileName = entry.Name;
			   String strDir = strFullName.Substring( 0, strFullName.Length - strFileName.Length );

			   writer.WriteAttributeString( "dir", strDir );
			   bFirst = false;
			}   

			// Add a new text node with a tag entry.  There will be one added per
			// item encountered in the directory.
			writer.WriteStartElement( "entry", "" );
		 
			// Write out the things we are interested in about this entry in the
			// directory.
			writer.WriteElementString( "name", entry.Name );
			writer.WriteElementString( "created", entry.CreationTime.ToString() );
			writer.WriteElementString( "lastaccess", entry.LastAccessTime.ToString() );
			writer.WriteElementString( "lastwrite", entry.LastWriteTime.ToString() );
			writer.WriteElementString( "isfile", ( (entry.Attributes & FileAttributes.Directory) > 0 ) ? "False" : "True" );
			writer.WriteElementString( "isdir", ( (entry.Attributes & FileAttributes.Directory) > 0 ) ? "True" : "False" );
			writer.WriteElementString( "readonly", ( (entry.Attributes & FileAttributes.ReadOnly) > 0 ) ? "True" : "False" );
   
			writer.WriteEndElement();    // entry     
		 }

		 writer.WriteEndElement();       // dirlist
		 writer.WriteEndDocument();

		 string strXml = writerString.ToString();
		 StringReader reader = new StringReader( strXml );
			   
		 XmlDocument doc = new XmlDocument();
		 doc.Load( reader );
		 
		 return doc;
	  } 
   }



}
