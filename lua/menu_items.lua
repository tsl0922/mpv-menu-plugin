local opt = require 'mp.options'
local msg = require 'mp.msg'

local opts = {
    uosc = false, -- uosc syntax support
}
local items = {}

opt.read_options(opts, 'menu')

function get_input_conf()
    local prop = mp.get_property_native('input-conf')
    if prop:sub(1, 9) == 'memory://' then return prop:sub(10) end

    prop = prop == '' and '~~/input.conf' or prop
    local conf_path = mp.command_native({ 'expand-path', prop })

    local f = io.open(conf_path, 'rb')
    if not f then return nil end
    local conf = f:read('*all')
    f:close()
    return conf
end

function build_track_items(list, type, prop)
    local submenu = {}
    local pos = mp.get_property_number(prop)
    for _, track in ipairs(list) do
        if track.type == type then
            local prefix = string.upper(track.type:sub(1, 1))
            local title = track.title or ('track ' .. track.id)
            local lang = track.lang and string.upper(track.lang) or ''

            local state = {}
            if track.selected then
                table.insert(state, 'checked')
                if track.id ~= pos then table.insert(state, 'disabled') end
            end

            submenu[#submenu + 1] = {
                title = string.format('%s: %s\t%s', prefix, title, lang),
                cmd = string.format('set %s %d', prop, track.id),
                state = state,
            }
        end
    end
    return submenu
end

function update_tracks_menu(submenu)
    mp.observe_property('track-list', 'native', function(_, track_list)
        for i = #submenu, 1, -1 do table.remove(submenu, i) end

        local items_v = build_track_items(track_list, 'video', 'vid')
        local items_a = build_track_items(track_list, 'audio', 'aid')
        local items_s = build_track_items(track_list, 'sub', 'sid')

        for _, item in ipairs(items_v) do table.insert(submenu, item) end
        if #submenu > 0 and #items_a > 0 then table.insert(submenu, { type = 'separator' }) end
        for _, item in ipairs(items_a) do table.insert(submenu, item) end
        if #submenu > 0 and #items_s > 0 then table.insert(submenu, { type = 'separator' }) end
        for _, item in ipairs(items_s) do table.insert(submenu, item) end

        mp.set_property_native("user-data/menu/items", items)
    end)
end

function update_chapters_menu(submenu)
    mp.observe_property('chapter-list', 'native', function(_, chapter_list)
        for i = #submenu, 1, -1 do table.remove(submenu, i) end

        local pos = mp.get_property_number('chapter')
        for id, chapter in ipairs(chapter_list) do
            local title = chapter.title or ('chapter ' .. id)
            local time = string.format('%02d:%02d:%02d', chapter.time / 3600, chapter.time / 60 % 60, chapter.time % 60)

            local state = {}
            if id == pos + 1 then table.insert(state, 'checked') end

            submenu[#submenu + 1] = {
                title = string.format('%s\t[%s]', title, time),
                cmd = string.format('seek %f absolute', chapter.time),
                state = state,
            }
        end

        mp.set_property_native("user-data/menu/items", items)
    end)

    mp.observe_property('chapter', 'number', function(_, pos)
        for id, item in ipairs(submenu) do
            item.state = id == pos + 1 and { 'checked' } or {}
        end
        mp.set_property_native("user-data/menu/items", items)
    end)
end

-- dynamic menu providers
local providers = {
    tracks = update_tracks_menu,
    chapters = update_chapters_menu,
}

function parse_input_conf(conf)
    local items = {}
    local by_id = {}

    for line in conf:gmatch('[^\r\n]+') do
        if line:sub(1, 1) ~= '#' or opts.uosc then
            -- split key, cmd and comment
            local key, cmd, comment = line:match('%s*([%S]+)%s+(.-)%s+#%s*(.-)%s*$')
            local title = ''
            local keyword = ''

            -- parse comment for menu title
            if comment and comment ~= '' then
                -- strip trailing comment
                local text, rest = comment:match('(.-)#%s*(.-)%s*$')
                if not text then text = comment end
                -- extract keyword for #@keyword syntax
                keyword = rest and rest:match('^@([%S]+).-%s*$') or ''
                -- extract menu title for #menu: syntax
                title = text:match('^menu:%s*(.*)%s*')
                -- extract menu title for #! syntax
                if not title and opts.uosc then
                    title = text:match('^!%s*(.*)%s*')
                end
            end

            -- split menu/submenu items separated by '>'
            if title and title ~= '' then
                local submenu_id = ''
                local target_menu = items

                local pattern = '(.-) *> *'
                local last_ends = 1
                local starts, ends, match = title:find(pattern)

                -- submenu items
                while starts do
                    submenu_id = submenu_id .. match
                    if not by_id[submenu_id] then
                        local submenu = {}
                        by_id[submenu_id] = submenu
                        target_menu[#target_menu + 1] = {
                            title = match,
                            type = 'submenu',
                            submenu = submenu,
                        }
                    end
                    target_menu = by_id[submenu_id]

                    last_ends = ends + 1
                    starts, ends, match = title:find(pattern, last_ends)
                end

                -- menu item
                if last_ends < (#title + 1) then
                    local text = title:sub(last_ends)
                    if text == '-' or (opts.uosc and text:sub(1, 3) == '---') then
                        target_menu[#target_menu + 1] = {
                            type = 'separator',
                        }
                    else
                        if keyword and keyword ~= '' then
                            local submenu = {}
                            target_menu[#target_menu + 1] = {
                                title = text,
                                type = 'submenu',
                                submenu = submenu,
                            }
                            if providers[keyword] then providers[keyword](submenu) end
                        else
                            target_menu[#target_menu + 1] = {
                                title = (key ~= '' and key ~= '_' and key ~= '#') and (text .. "\t" .. key) or text,
                                cmd = cmd,
                            }
                        end
                    end
                end
            end
        end
    end

    return items
end

local conf = get_input_conf()

if conf then
    items = parse_input_conf(conf)
    mp.set_property_native("user-data/menu/items", items)
end
