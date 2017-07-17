Imports System.IO
Imports System.Threading
Imports System.IO.MemoryMappedFiles
Imports System.IO.File
Imports System.IO.Pipes
Imports System.Math
Imports System.Security.Principal
Imports System.Security.AccessControl


Public Class RotarySwitchControl
    Private DEFINE_COLOR_KNOB_CUSTOM_ON As Color = Color.FromArgb(&HFF, &H66, &H66, &H66)
    Private DEFINE_COLOR_KNOB_CUSTOM_OFF As Color = Color.FromArgb(&HFF, &H33, &H33, &H33)
    Private DEFINE_COLOR_KNOB_CUSTOM_CURSOR As Color = Color.Red

    Private DEFINE_KNOB_CURSOR_MARGIN_RATIO As Single = 50
    Private DEFINE_KNOB_PULSE_NUMBER As Single = 15

    Private DEFINE_COLOR_KNOB As Color = Color.Black
    '
    Private m_isMouseDown As Boolean = False
    Private m_isMouseOver As Boolean = False
    Private m_mouseLocation As Point
    '
    Private m_isActive As Boolean = False

    '
    Private m_diameter As Single
    Private m_center_x As Single
    Private m_center_y As Single
    '
    Private m_cursor_diameter As Single
    Private m_cursor_center_x As Single
    Private m_cursor_center_y As Single
    '
    Private m_resolution_angle As Single
    Private m_current_angle As Single
    '
    Private m_BrushOn As Brush
    Private m_BrushOff As Brush
    '
    Private m_BrushCursor As Brush
    '
    Private m_event_mmap As MemoryMappedFile = Nothing
    Private m_event_semaphore As Semaphore = Nothing
    Private m_event_stream As MemoryMappedViewStream = Nothing
    Private m_event_stream_writer As BinaryWriter = Nothing
    '
    Private m_pipe_security As PipeSecurity
    '
    Private m_event_pipe_stream_server As NamedPipeServerStream
    Private m_event_pipe_stream_writer As StreamWriter
    Private m_thread_pipe_server As Thread

    '
    Private Sub ThreadProcPipeServerWaitConnection()
        '

        If (Not m_event_pipe_stream_server.IsConnected()) Then
            m_event_pipe_stream_server.WaitForConnection()
        End If
        '
        m_event_pipe_stream_writer = New StreamWriter(m_event_pipe_stream_server)
        m_event_pipe_stream_writer.AutoFlush = True
        '
        'Debug.Print("wait disconnection...")
        'Dim v As Integer = m_event_pipe_stream_server.ReadByte()
        ' 
        'Debug.Print("disconnected")
        '
    End Sub
    '
    Private Sub CalculateDimension()


        'knob diameter
        If (Me.Width <= Me.Height) Then
            m_diameter = Me.Width
        Else
            m_diameter = Me.Height
        End If
        'cursor diameter
        m_cursor_diameter = m_diameter / 10
        '
        m_center_x = Me.Width / 2
        m_center_y = Me.Height / 2

        '
        m_resolution_angle = Math.PI / DEFINE_KNOB_PULSE_NUMBER
        '
        Dim n_pulse As Integer = (Math.PI / 4) / m_resolution_angle
        m_current_angle = n_pulse * m_resolution_angle
        '
        m_cursor_center_x = m_center_x + ((m_diameter / 2 - m_cursor_diameter / 2) - m_diameter / DEFINE_KNOB_CURSOR_MARGIN_RATIO) * Cos(m_current_angle)
        m_cursor_center_y = m_center_y - ((m_diameter / 2 - m_cursor_diameter / 2) - m_diameter / DEFINE_KNOB_CURSOR_MARGIN_RATIO) * Cos(m_current_angle)


    End Sub

    '
    Private Sub RotarySwitch_Paint(sender As Object, e As PaintEventArgs) Handles Me.Paint
        Dim g As Graphics = e.Graphics

        If (m_isMouseOver And m_isMouseDown) Then
            '
            m_isActive = True
            '
            g.FillEllipse(m_BrushOn, m_center_x - m_diameter / 2, m_center_y - m_diameter / 2, m_diameter, m_diameter)
            '
            g.FillEllipse(m_BrushCursor, m_cursor_center_x - m_cursor_diameter / 2, m_cursor_center_y - m_cursor_diameter / 2, m_cursor_diameter, m_cursor_diameter)
            '
        Else
            '
            m_isActive = False
            '
            g.FillEllipse(m_BrushOff, m_center_x - m_diameter / 2, m_center_y - m_diameter / 2, m_diameter, m_diameter)
            '
            g.FillEllipse(m_BrushCursor, m_cursor_center_x - m_cursor_diameter / 2, m_cursor_center_y - m_cursor_diameter / 2, m_cursor_diameter, m_cursor_diameter)
            '
        End If
    End Sub
    '
    Private Sub UserControl1_Load(sender As Object, e As EventArgs) Handles Me.Load
        m_BrushOn = New SolidBrush(DEFINE_COLOR_KNOB_CUSTOM_ON)
        m_BrushOff = New SolidBrush(DEFINE_COLOR_KNOB_CUSTOM_OFF)
        m_BrushCursor = New SolidBrush(DEFINE_COLOR_KNOB_CUSTOM_CURSOR)
        '
        CalculateDimension()
        '
        Me.DoubleBuffered = True
        '
        If (Not Me.DesignMode()) Then
            Dim semaphoreCreated As Boolean
            m_event_mmap = System.IO.MemoryMappedFiles.MemoryMappedFile.CreateNew(Me.Name & ".mmap", 256)
            m_event_semaphore = New Semaphore(0, 1, Me.Name & ".sem", semaphoreCreated)
            '
            m_event_stream = m_event_mmap.CreateViewStream()
            m_event_stream_writer = New BinaryWriter(m_event_stream)
            '
            m_event_pipe_stream_server = New NamedPipeServerStream(Me.Name & ".pipe", PipeDirection.InOut, 1, PipeTransmissionMode.Byte)

            ' Provide full access to the current user so more pipe instances can be created
#If 0 Then
            m_pipe_security = m_event_pipe_stream_server.GetAccessControl()

            '
            m_pipe_security.AddAccessRule(
                New PipeAccessRule(WindowsIdentity.GetCurrent().User, PipeAccessRights.FullControl, AccessControlType.Allow)
            )

            '
            Dim securityIentifier As SecurityIdentifier = New SecurityIdentifier(WellKnownSidType.AuthenticatedUserSid, Nothing)
            m_pipe_security.AddAccessRule(
                New PipeAccessRule(securityIentifier, PipeAccessRights.ReadWrite, AccessControlType.Allow)
            )

            '
            m_event_pipe_stream_server.SetAccessControl(m_pipe_security)
#End If
            '
            m_thread_pipe_server = New Thread(New ThreadStart(AddressOf Me.ThreadProcPipeServerWaitConnection))
            '
            m_thread_pipe_server.Start()

        End If
        '
    End Sub
    '
    Private Sub RotarySwitch_Resize(sender As Object, e As EventArgs) Handles Me.Resize
        CalculateDimension()
    End Sub
    '
    Private Sub RotarySwitch_MouseDown(sender As Object, e As MouseEventArgs) Handles Me.MouseDown
        '
        If (m_isMouseDown = True) Then
            Exit Sub
        End If
        '
        m_isMouseDown = True
        '
        Me.Invalidate()
        '
    End Sub
    '
    Private Sub RotarySwitch_MouseUp(sender As Object, e As MouseEventArgs) Handles Me.MouseUp
        '
        If (m_isMouseDown = False) Then
            Exit Sub
        End If
        '
        m_isMouseDown = False
        '
        Me.Invalidate()
        '
    End Sub
    '
    Private Sub RotarySwitch_MouseEnter(sender As Object, e As EventArgs) Handles Me.MouseEnter
        '
        If (m_isMouseOver = True) Then
            Exit Sub
        End If
        '
        m_isMouseOver = True
        '
        Me.Invalidate()
        '
    End Sub
    '
    Private Sub RotarySwitch_MouseLeave(sender As Object, e As EventArgs) Handles Me.MouseLeave
        '
        If (m_isMouseOver = False) Then
            Exit Sub
        End If
        '
        m_isMouseOver = False
        '
        Me.Invalidate()
        '
    End Sub
    '
    Private Sub RotarySwitch_MouseMove(sender As Object, e As MouseEventArgs) Handles Me.MouseMove
        '
        Dim increment As SByte = 1 '+1,-1
        Dim angle As Single = Math.PI / 4
        '
        m_mouseLocation = e.Location
        '
        If (Not m_isActive) Then
            Exit Sub
        End If
        '
        Dim from_center_pos_x As Single = (m_mouseLocation.X - m_center_x)
        Dim from_center_pos_y As Single = (m_center_y - m_mouseLocation.Y)
        '
        If (from_center_pos_x <> 0) Then
            If (from_center_pos_x > 0) Then
                angle = Atan(from_center_pos_y / from_center_pos_x)
            Else
                angle = Math.PI + Atan(from_center_pos_y / from_center_pos_x)
            End If
        ElseIf (from_center_pos_x = 0 And from_center_pos_y > 0) Then
            angle = PI / 2
        ElseIf (from_center_pos_x = 0 And from_center_pos_y < 0) Then
            angle = -PI / 2
        End If
        '
        If (angle < 0) Then
            angle = (2.0 * Math.PI) + angle
        End If

        '
        Dim n_pulse As Integer = (angle) / m_resolution_angle
        angle = n_pulse * m_resolution_angle

        '
        If (angle < Math.PI / 2 And m_current_angle > (3 * Math.PI / 2)) Then
            increment = -1
        ElseIf (m_current_angle < Math.PI / 2 And angle > (3 * Math.PI / 2)) Then
            increment = +1
        ElseIf ((angle - m_current_angle) < 0) Then
            increment = +1
        ElseIf ((angle - m_current_angle) > 0) Then
            increment = -1
        Else
            Return
        End If

        Debug.Print("rotation: " & increment)
        '
#If 0 Then
        Try
            'm_event_stream_writer.Write(increment)
        Catch ex As Exception
            'nothing to do
        End Try
#End If
        '
        If (Not m_event_pipe_stream_server Is Nothing And m_event_pipe_stream_server.IsConnected()) Then
            If (Not m_event_pipe_stream_writer Is Nothing) Then
                'm_event_pipe_stream_writer.Write(increment)
                Dim uByte As Byte
                uByte = CByte(increment And &HFF) ' result: 254
                Try
                    m_event_pipe_stream_server.WriteByte(uByte)
                Catch ex As Exception
                    'do nothing
                End Try

            End If
        ElseIf (Not m_event_pipe_stream_server Is Nothing And m_event_pipe_stream_server.IsConnected()) Then
            If (Not m_thread_pipe_server.IsAlive) Then
                m_thread_pipe_server = New Thread(New ThreadStart(AddressOf Me.ThreadProcPipeServerWaitConnection))
                '
                m_thread_pipe_server.Start()
            End If

        End If

        '
        m_cursor_center_x = m_center_x + ((m_diameter / 2 - m_cursor_diameter / 2) - m_diameter / DEFINE_KNOB_CURSOR_MARGIN_RATIO) * Cos(angle)
        m_cursor_center_y = m_center_y - ((m_diameter / 2 - m_cursor_diameter / 2) - m_diameter / DEFINE_KNOB_CURSOR_MARGIN_RATIO) * Sin(angle)
        '
        m_current_angle = angle
        '
        Me.Invalidate()
        '
    End Sub


#If 0 Then
    Private Sub RotarySwitch_MouseHover(sender As Object, e As EventArgs) Handles Me.MouseHover
        m_isMouseOver = True
        Me.Invalidate()
    End Sub
#End If
End Class
