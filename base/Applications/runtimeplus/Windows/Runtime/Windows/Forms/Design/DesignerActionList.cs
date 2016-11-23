/// Copyright (c) 2015 Jonathan Moore
///
/// This software is provided 'as-is', without any express or implied warranty. 
/// In no event will the authors be held liable for any damages arising from 
/// the use of this software.
/// 
/// Permission is granted to anyone to use this software for any purpose, 
/// including commercial applications, and to alter it and redistribute it 
/// freely, subject to the following restrictions:
///
/// 1. The origin of this software must not be misrepresented; 
/// you must not claim that you wrote the original software. 
/// If you use this software in a product, an acknowledgment in the product 
/// documentation would be appreciated but is not required.
/// 
/// 2. Altered source versions must be plainly marked as such, 
/// and must not be misrepresented as being the original software.
///
///3. This notice may not be removed or altered from any source distribution.

namespace System.Extensions.Windows.Forms.Design
{
  using System.ComponentModel;
  using System.ComponentModel.Design;

  /// <summary>
  /// Provides the base class for types that define a list of items used to create a smart tag panel.
  /// </summary>
  public abstract class DesignerActionList : System.ComponentModel.Design.DesignerActionList
  {
    /// <summary>
    /// Initializes a new instance of the <see cref="DesignerActionList"/> class.
    /// </summary>
    /// <param name="component">A component related to this DesignerActionList.</param>
    protected DesignerActionList(IComponent component)
      : base(component)
    {
    }

    /// <summary>
    /// Replaces the selection with the component this list is attached to.
    /// </summary>
    protected void SelectComponent()
    {
      IComponent component = this.Component;
      if (component == null)
      {
        return;
      }

      ISelectionService service = (ISelectionService)GetService(typeof(ISelectionService));
      if (service == null)
      {
        return;
      }

      object[] components = new object[] { component };
      service.SetSelectedComponents(components, SelectionTypes.Replace);
    }

    /// <summary>
    /// Removes the component this list is attached to from the selection.
    /// </summary>
    protected void DeselectComponent()
    {
      IComponent component = this.Component;
      if (component == null)
      {
        return;
      }

      ISelectionService service = (ISelectionService)GetService(typeof(ISelectionService));
      if (service == null)
      {
        return;
      }

      object[] components = new object[] { component };
      service.SetSelectedComponents(components, SelectionTypes.Remove);
    }
  }
}
