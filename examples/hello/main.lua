function poti.load()
    print("testandow")
end

function poti.draw()
    poti.render.clear()
    poti.render.color(255, 255, 255)
    poti.render.rectangle(0, 0, 32, 32)
end

function poti.draw_gui()
    if poti.gui.begin_window("boraveeee", {0, 0, 250, 150}) then
        poti.gui.button("opa")
        poti.gui.end_window()
    end
end
