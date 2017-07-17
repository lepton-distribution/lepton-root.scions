<Global.Microsoft.VisualBasic.CompilerServices.DesignerGenerated()> _
Partial Class Form1
    Inherits System.Windows.Forms.Form

    'Form remplace la méthode Dispose pour nettoyer la liste des composants.
    <System.Diagnostics.DebuggerNonUserCode()> _
    Protected Overrides Sub Dispose(ByVal disposing As Boolean)
        Try
            If disposing AndAlso components IsNot Nothing Then
                components.Dispose()
            End If
        Finally
            MyBase.Dispose(disposing)
        End Try
    End Sub

    'Requise par le Concepteur Windows Form
    Private components As System.ComponentModel.IContainer

    'REMARQUE : la procédure suivante est requise par le Concepteur Windows Form
    'Elle peut être modifiée à l'aide du Concepteur Windows Form.  
    'Ne la modifiez pas à l'aide de l'éditeur de code.
    <System.Diagnostics.DebuggerStepThrough()> _
    Private Sub InitializeComponent()
        Me.LcdMatrixControl1 = New WindowsControlLibrary.LcdMatrixControl()
        Me.RotarySwitchControl1 = New WindowsControlLibrary.RotarySwitchControl()
        Me.RotarySwitchControl2 = New WindowsControlLibrary.RotarySwitchControl()
        Me.RotarySwitchControl3 = New WindowsControlLibrary.RotarySwitchControl()
        Me.RotarySwitchControl4 = New WindowsControlLibrary.RotarySwitchControl()
        Me.SuspendLayout()
        '
        'LcdMatrixControl1
        '
        Me.LcdMatrixControl1.Location = New System.Drawing.Point(35, 27)
        Me.LcdMatrixControl1.Name = "LcdMatrixControl1"
        Me.LcdMatrixControl1.Size = New System.Drawing.Size(514, 130)
        Me.LcdMatrixControl1.TabIndex = 0
        '
        'RotarySwitchControl1
        '
        Me.RotarySwitchControl1.Location = New System.Drawing.Point(35, 187)
        Me.RotarySwitchControl1.Name = "RotarySwitchControl1"
        Me.RotarySwitchControl1.Size = New System.Drawing.Size(92, 56)
        Me.RotarySwitchControl1.TabIndex = 1
        '
        'RotarySwitchControl2
        '
        Me.RotarySwitchControl2.Location = New System.Drawing.Point(181, 187)
        Me.RotarySwitchControl2.Name = "RotarySwitchControl2"
        Me.RotarySwitchControl2.Size = New System.Drawing.Size(92, 56)
        Me.RotarySwitchControl2.TabIndex = 2
        '
        'RotarySwitchControl3
        '
        Me.RotarySwitchControl3.Location = New System.Drawing.Point(315, 187)
        Me.RotarySwitchControl3.Name = "RotarySwitchControl3"
        Me.RotarySwitchControl3.Size = New System.Drawing.Size(92, 56)
        Me.RotarySwitchControl3.TabIndex = 3
        '
        'RotarySwitchControl4
        '
        Me.RotarySwitchControl4.Location = New System.Drawing.Point(457, 187)
        Me.RotarySwitchControl4.Name = "RotarySwitchControl4"
        Me.RotarySwitchControl4.Size = New System.Drawing.Size(92, 56)
        Me.RotarySwitchControl4.TabIndex = 4
        '
        'Form1
        '
        Me.AutoScaleDimensions = New System.Drawing.SizeF(11.0!, 24.0!)
        Me.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font
        Me.ClientSize = New System.Drawing.Size(614, 271)
        Me.Controls.Add(Me.RotarySwitchControl4)
        Me.Controls.Add(Me.RotarySwitchControl3)
        Me.Controls.Add(Me.RotarySwitchControl2)
        Me.Controls.Add(Me.RotarySwitchControl1)
        Me.Controls.Add(Me.LcdMatrixControl1)
        Me.Name = "Form1"
        Me.Text = "Form1"
        Me.ResumeLayout(False)

    End Sub

    Friend WithEvents LcdMatrixControl1 As WindowsControlLibrary.LcdMatrixControl
    Friend WithEvents RotarySwitchControl1 As WindowsControlLibrary.RotarySwitchControl
    Friend WithEvents RotarySwitchControl2 As WindowsControlLibrary.RotarySwitchControl
    Friend WithEvents RotarySwitchControl3 As WindowsControlLibrary.RotarySwitchControl
    Friend WithEvents RotarySwitchControl4 As WindowsControlLibrary.RotarySwitchControl
End Class
